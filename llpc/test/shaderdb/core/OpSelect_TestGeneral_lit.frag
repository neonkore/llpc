// NOTE: Assertions have been autogenerated by tool/update_llpc_test_checks.py
// RUN: amdllpc -emit-lgc -gfxip 10.3 -o - %s | FileCheck -check-prefix=SHADERTEST %s

#version 450

layout(binding = 0) uniform Uniforms
{
    bool cond;
};

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(cond);
}

// SHADERTEST-LABEL: @lgc.shader.FS.main(
// SHADERTEST-NEXT:  .entry:
// SHADERTEST-NEXT:    [[TMP0:%.*]] = call ptr addrspace(7) @lgc.load.buffer.desc(i64 0, i32 0, i32 0, i32 0)
// SHADERTEST-NEXT:    [[TMP1:%.*]] = call ptr @llvm.invariant.start.p7(i64 -1, ptr addrspace(7) [[TMP0]])
// SHADERTEST-NEXT:    [[TMP2:%.*]] = load i32, ptr addrspace(7) [[TMP0]], align 4
// SHADERTEST-NEXT:    [[DOTNOT:%.*]] = icmp eq i32 [[TMP2]], 0
// SHADERTEST-NEXT:    [[TMP3:%.*]] = select reassoc nnan nsz arcp contract afn i1 [[DOTNOT]], float 0.000000e+00, float 1.000000e+00
// SHADERTEST-NEXT:    [[TMP4:%.*]] = insertelement <4 x float> poison, float [[TMP3]], i64 0
// SHADERTEST-NEXT:    [[TMP5:%.*]] = shufflevector <4 x float> [[TMP4]], <4 x float> poison, <4 x i32> zeroinitializer
// SHADERTEST-NEXT:    call void (...) @lgc.create.write.generic.output(<4 x float> [[TMP5]], i32 0, i32 0, i32 0, i32 0, i32 0, i32 poison)
// SHADERTEST-NEXT:    ret void
//
