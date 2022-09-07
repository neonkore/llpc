/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2022 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************************************************/
/**
 ***********************************************************************************************************************
 * @file  MeshTaskShader.h
 * @brief LLPC header file: contains declaration of class lgc::MeshTaskShader.
 ***********************************************************************************************************************
 */
#pragma once

#include "lgc/patch/SystemValues.h"
#include "lgc/state/PipelineState.h"
#include "lgc/state/TargetInfo.h"

namespace lgc {

// Represents the entry layout of mesh pipeline statistics buffer for a workgroup
struct MeshPipeStatsEntry {
  uint64_t numMeshThreads;
  uint64_t numMeshPrimitives;
  uint64_t numTaskThreads;
};

// Enumerates the LDS regions used by mesh shader
enum class MeshLdsRegion : unsigned {
  VertexCount = 0,  // Vertex count set by SetMeshOutputs
  PrimitiveCount,   // Primitive count set by SetMeshOutputs
  FlatWorkgroupId,  // Flat workgroup ID
  PrimitiveIndices, // Primitive indices set by SetPrimitiveIndices
  VertexOutput,     // Per-vertex outputs
  PrimitiveOutput,  // Per-primitive outputsr
};

// Map: LDS Region -> <Region Offset, Region Size>
typedef std::unordered_map<MeshLdsRegion, std::pair<unsigned, unsigned>> MeshLdsLayout;

// =====================================================================================================================
// Represents the handler of mesh/task shader.
class MeshTaskShader {
public:
  MeshTaskShader(PipelineState *pipelineState);
  ~MeshTaskShader();

  static unsigned layoutMeshShaderLds(PipelineState *pipelineState, llvm::Function *entryPoint,
                                      MeshLdsLayout *ldsLayout = nullptr);

  void process(llvm::Function *taskEntryPoint, llvm::Function *meshEntryPoint);

private:
  static llvm::GlobalVariable *getOrCreateMeshLds(llvm::Module *module, unsigned meshLdsSizeInDwords = 0);
  static unsigned useFlatWorkgroupId(PipelineState *pipelineState);

  void processTaskShader(llvm::Function *entryPoint);
  void processMeshShader(llvm::Function *entryPoint);

  llvm::Value *readTaskPayload(llvm::Type *readTy, llvm::Value *byteOffset);
  void writeTaskPayload(llvm::Value *writeValue, llvm::Value *byteOffset);
  llvm::Value *taskPayloadAtomic(unsigned atomicOp, llvm::AtomicOrdering ordering, llvm::Value *inputValue,
                                 llvm::Value *byteOffset);
  llvm::Value *taskPayloadAtomicCompareSwap(llvm::AtomicOrdering ordering, llvm::Value *inputValue,
                                            llvm::Value *comparatorValue, llvm::Value *byteOffset);

  void initWaveThreadInfo(llvm::Function *entryPoint);
  llvm::Value *getShaderRingEntryIndex(llvm::Function *entryPoint);

  llvm::Value *getPayloadRingEntryOffset(llvm::Function *entryPoint);
  llvm::Value *getDrawDataRingEntryOffset(llvm::Function *entryPoint);
  llvm::Value *getDrawDataReadyBit(llvm::Function *entryPoint);
  void emitTaskMeshs(llvm::Value *groupCountX, llvm::Value *groupCountY, llvm::Value *groupCountZ);

  llvm::Value *convertToDivergent(llvm::Value *value);

  llvm::Function *mutateMeshShaderEntryPoint(llvm::Function *entryPoint);
  void lowerMeshShaderBody(llvm::BasicBlock *beginMeshShaderBlock);
  void setMeshOutputs(llvm::Value *vertexCount, llvm::Value *primitiveCount);
  void setPrimitiveIndices(llvm::Value *primitiveIndex, llvm::Value *primitiveIndices);
  void setPrimitiveCulled(llvm::Value *primitiveIndex, llvm::Value *isCulled);
  llvm::Value *getMeshInput(BuiltInKind builtIn);
  void writeVertexOutput(llvm::Value *outputOffset, llvm::Value *vertexIndex, llvm::Value *outputValue);
  void writePrimitiveOutput(llvm::Value *outputOffset, llvm::Value *primitiveIndex, llvm::Value *outputValue);

  void exportPrimitive();
  void exportVertex();
  void collectMeshStatsInfo(llvm::Function *entryPoint, llvm::Value *numMeshPrimitives);

  llvm::Value *getMeshFlatWorkgroupId();
  llvm::Value *getMeshNumWorkgroups();
  llvm::Value *getMeshWorkgroupId();
  llvm::Value *getMeshLocalInvocationId();
  llvm::Value *getMeshLocalInvocationIndex();
  llvm::Value *getMeshGlobalInvocationId();
  llvm::Value *getGlobalInvocationIndex();

  llvm::Value *readMeshBuiltInFromLds(BuiltInKind builtIn);
  std::pair<llvm::Value *, llvm::Value *> convertToHwShadingRate(llvm::Value *primitiveShadingRate);

  unsigned getMeshShaderLdsRegionStart(MeshLdsRegion region) {
    assert(m_ldsLayout.count(region) > 0);
    return m_ldsLayout[region].first;
  }

  llvm::Value *readValueFromLds(llvm::Type *readTy, llvm::Value *ldsOffset);
  void writeValueToLds(llvm::Value *writeValue, llvm::Value *ldsOffset);
  void atomicOpWithLds(llvm::AtomicRMWInst::BinOp atomicOp, llvm::Value *atomicValue, llvm::Value *ldsOffset);

  static constexpr unsigned PayloadRingEntrySize = 16 * 1024; // 16K bytes per group
  static constexpr unsigned DrawDataRingEntrySize = 16;       // 16 bytes per group

  PipelineState *m_pipelineState = nullptr; // Pipeline state

  PipelineSystemValues m_pipelineSysValues; // Cache of ShaderSystemValues objects, one per shader stage

  std::unique_ptr<llvm::IRBuilder<>> m_builder; // LLVM IR builder

  // The wave/thread info used for control shader branching
  struct {
    llvm::Value *waveIdInSubgroup;
    llvm::Value *threadIdInWave;
    llvm::Value *threadIdInSubgroup;
  } m_waveThreadInfo = {};

  bool m_accessTaskPayload = false;                // Whether task shader has payload access operations
  llvm::Value *m_shaderRingEntryIndex = nullptr;   // Shader ring entry index of current workgroup
  llvm::Value *m_payloadRingEntryOffset = nullptr; // Entry offset (in bytes) of the payload ring

  llvm::Value *m_meshFlatWorkgroupId = nullptr;       // Flat workgroupId of mesh shader
  llvm::Value *m_meshWorkgroupId = nullptr;           // Built-in WorkgroupId of mesh shader
  llvm::Value *m_meshLocalInvocationId = nullptr;     // Built-in LocalInvocationId of mesh shader
  llvm::Value *m_meshGlobalInvocationId = nullptr;    // Built-in GlobalInvocationId of mesh shader
  llvm::Value *m_meshGlobalInvocationIndex = nullptr; // Global invocation index of mesh shader

  llvm::GlobalValue *m_lds = nullptr; // Global variable to model mesh shader LDS

  GfxIpVersion m_gfxIp; // Graphics IP version info

  MeshLdsLayout m_ldsLayout; // Mesh shader LDS layout
};

} // namespace lgc
