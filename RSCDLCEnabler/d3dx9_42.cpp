#include <windows.h>
#pragma pack(1)


FARPROC p[329] = { 0 };
HINSTANCE hL;

void ShutdownProxy() {
	FreeLibrary(hL);
}

bool InitProxy() {
	char winpath[MAX_PATH];

	if (hL)
		return(TRUE);

	GetSystemDirectoryA(winpath, sizeof(winpath));
	strcat_s(winpath, "\\D3DX9_42.dll");

	hL = LoadLibraryA(winpath);
	if (!hL)
		return(FALSE);

	p[0] = GetProcAddress(hL, "D3DXAssembleShader");
	p[1] = GetProcAddress(hL, "D3DXAssembleShaderFromFileA");
	p[2] = GetProcAddress(hL, "D3DXAssembleShaderFromFileW");
	p[3] = GetProcAddress(hL, "D3DXAssembleShaderFromResourceA");
	p[4] = GetProcAddress(hL, "D3DXAssembleShaderFromResourceW");
	p[5] = GetProcAddress(hL, "D3DXBoxBoundProbe");
	p[6] = GetProcAddress(hL, "D3DXCheckCubeTextureRequirements");
	p[7] = GetProcAddress(hL, "D3DXCheckTextureRequirements");
	p[8] = GetProcAddress(hL, "D3DXCheckVersion");
	p[9] = GetProcAddress(hL, "D3DXCheckVolumeTextureRequirements");
	p[10] = GetProcAddress(hL, "D3DXCleanMesh");
	p[11] = GetProcAddress(hL, "D3DXColorAdjustContrast");
	p[12] = GetProcAddress(hL, "D3DXColorAdjustSaturation");
	p[13] = GetProcAddress(hL, "D3DXCompileShader");
	p[14] = GetProcAddress(hL, "D3DXCompileShaderFromFileA");
	p[15] = GetProcAddress(hL, "D3DXCompileShaderFromFileW");
	p[16] = GetProcAddress(hL, "D3DXCompileShaderFromResourceA");
	p[17] = GetProcAddress(hL, "D3DXCompileShaderFromResourceW");
	p[18] = GetProcAddress(hL, "D3DXComputeBoundingBox");
	p[19] = GetProcAddress(hL, "D3DXComputeBoundingSphere");
	p[20] = GetProcAddress(hL, "D3DXComputeIMTFromPerTexelSignal");
	p[21] = GetProcAddress(hL, "D3DXComputeIMTFromPerVertexSignal");
	p[22] = GetProcAddress(hL, "D3DXComputeIMTFromSignal");
	p[23] = GetProcAddress(hL, "D3DXComputeIMTFromTexture");
	p[24] = GetProcAddress(hL, "D3DXComputeNormalMap");
	p[25] = GetProcAddress(hL, "D3DXComputeNormals");
	p[26] = GetProcAddress(hL, "D3DXComputeTangent");
	p[27] = GetProcAddress(hL, "D3DXComputeTangentFrame");
	p[28] = GetProcAddress(hL, "D3DXComputeTangentFrameEx");
	p[29] = GetProcAddress(hL, "D3DXConcatenateMeshes");
	p[30] = GetProcAddress(hL, "D3DXConvertMeshSubsetToSingleStrip");
	p[31] = GetProcAddress(hL, "D3DXConvertMeshSubsetToStrips");
	p[32] = GetProcAddress(hL, "D3DXCreateAnimationController");
	p[33] = GetProcAddress(hL, "D3DXCreateBox");
	p[34] = GetProcAddress(hL, "D3DXCreateBuffer");
	p[35] = GetProcAddress(hL, "D3DXCreateCompressedAnimationSet");
	p[36] = GetProcAddress(hL, "D3DXCreateCubeTexture");
	p[37] = GetProcAddress(hL, "D3DXCreateCubeTextureFromFileA");
	p[38] = GetProcAddress(hL, "D3DXCreateCubeTextureFromFileExA");
	p[39] = GetProcAddress(hL, "D3DXCreateCubeTextureFromFileExW");
	p[40] = GetProcAddress(hL, "D3DXCreateCubeTextureFromFileInMemory");
	p[41] = GetProcAddress(hL, "D3DXCreateCubeTextureFromFileInMemoryEx");
	p[42] = GetProcAddress(hL, "D3DXCreateCubeTextureFromFileW");
	p[43] = GetProcAddress(hL, "D3DXCreateCubeTextureFromResourceA");
	p[44] = GetProcAddress(hL, "D3DXCreateCubeTextureFromResourceExA");
	p[45] = GetProcAddress(hL, "D3DXCreateCubeTextureFromResourceExW");
	p[46] = GetProcAddress(hL, "D3DXCreateCubeTextureFromResourceW");
	p[47] = GetProcAddress(hL, "D3DXCreateCylinder");
	p[48] = GetProcAddress(hL, "D3DXCreateEffect");
	p[49] = GetProcAddress(hL, "D3DXCreateEffectCompiler");
	p[50] = GetProcAddress(hL, "D3DXCreateEffectCompilerFromFileA");
	p[51] = GetProcAddress(hL, "D3DXCreateEffectCompilerFromFileW");
	p[52] = GetProcAddress(hL, "D3DXCreateEffectCompilerFromResourceA");
	p[53] = GetProcAddress(hL, "D3DXCreateEffectCompilerFromResourceW");
	p[54] = GetProcAddress(hL, "D3DXCreateEffectEx");
	p[55] = GetProcAddress(hL, "D3DXCreateEffectFromFileA");
	p[56] = GetProcAddress(hL, "D3DXCreateEffectFromFileExA");
	p[57] = GetProcAddress(hL, "D3DXCreateEffectFromFileExW");
	p[58] = GetProcAddress(hL, "D3DXCreateEffectFromFileW");
	p[59] = GetProcAddress(hL, "D3DXCreateEffectFromResourceA");
	p[60] = GetProcAddress(hL, "D3DXCreateEffectFromResourceExA");
	p[61] = GetProcAddress(hL, "D3DXCreateEffectFromResourceExW");
	p[62] = GetProcAddress(hL, "D3DXCreateEffectFromResourceW");
	p[63] = GetProcAddress(hL, "D3DXCreateEffectPool");
	p[64] = GetProcAddress(hL, "D3DXCreateFontA");
	p[65] = GetProcAddress(hL, "D3DXCreateFontIndirectA");
	p[66] = GetProcAddress(hL, "D3DXCreateFontIndirectW");
	p[67] = GetProcAddress(hL, "D3DXCreateFontW");
	p[68] = GetProcAddress(hL, "D3DXCreateKeyframedAnimationSet");
	p[69] = GetProcAddress(hL, "D3DXCreateLine");
	p[70] = GetProcAddress(hL, "D3DXCreateMatrixStack");
	p[71] = GetProcAddress(hL, "D3DXCreateMesh");
	p[72] = GetProcAddress(hL, "D3DXCreateMeshFVF");
	p[73] = GetProcAddress(hL, "D3DXCreateNPatchMesh");
	p[74] = GetProcAddress(hL, "D3DXCreatePMeshFromStream");
	p[75] = GetProcAddress(hL, "D3DXCreatePRTBuffer");
	p[76] = GetProcAddress(hL, "D3DXCreatePRTBufferTex");
	p[77] = GetProcAddress(hL, "D3DXCreatePRTCompBuffer");
	p[78] = GetProcAddress(hL, "D3DXCreatePRTEngine");
	p[79] = GetProcAddress(hL, "D3DXCreatePatchMesh");
	p[80] = GetProcAddress(hL, "D3DXCreatePolygon");
	p[81] = GetProcAddress(hL, "D3DXCreateRenderToEnvMap");
	p[82] = GetProcAddress(hL, "D3DXCreateRenderToSurface");
	p[83] = GetProcAddress(hL, "D3DXCreateSPMesh");
	p[84] = GetProcAddress(hL, "D3DXCreateSkinInfo");
	p[85] = GetProcAddress(hL, "D3DXCreateSkinInfoFVF");
	p[86] = GetProcAddress(hL, "D3DXCreateSkinInfoFromBlendedMesh");
	p[87] = GetProcAddress(hL, "D3DXCreateSphere");
	p[88] = GetProcAddress(hL, "D3DXCreateSprite");
	p[89] = GetProcAddress(hL, "D3DXCreateTeapot");
	p[90] = GetProcAddress(hL, "D3DXCreateTextA");
	p[91] = GetProcAddress(hL, "D3DXCreateTextW");
	p[92] = GetProcAddress(hL, "D3DXCreateTexture");
	p[93] = GetProcAddress(hL, "D3DXCreateTextureFromFileA");
	p[94] = GetProcAddress(hL, "D3DXCreateTextureFromFileExA");
	p[95] = GetProcAddress(hL, "D3DXCreateTextureFromFileExW");
	p[96] = GetProcAddress(hL, "D3DXCreateTextureFromFileInMemory");
	p[97] = GetProcAddress(hL, "D3DXCreateTextureFromFileInMemoryEx");
	p[98] = GetProcAddress(hL, "D3DXCreateTextureFromFileW");
	p[99] = GetProcAddress(hL, "D3DXCreateTextureFromResourceA");
	p[100] = GetProcAddress(hL, "D3DXCreateTextureFromResourceExA");
	p[101] = GetProcAddress(hL, "D3DXCreateTextureFromResourceExW");
	p[102] = GetProcAddress(hL, "D3DXCreateTextureFromResourceW");
	p[103] = GetProcAddress(hL, "D3DXCreateTextureGutterHelper");
	p[104] = GetProcAddress(hL, "D3DXCreateTextureShader");
	p[105] = GetProcAddress(hL, "D3DXCreateTorus");
	p[106] = GetProcAddress(hL, "D3DXCreateVolumeTexture");
	p[107] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromFileA");
	p[108] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromFileExA");
	p[109] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromFileExW");
	p[110] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromFileInMemory");
	p[111] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromFileInMemoryEx");
	p[112] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromFileW");
	p[113] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromResourceA");
	p[114] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromResourceExA");
	p[115] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromResourceExW");
	p[116] = GetProcAddress(hL, "D3DXCreateVolumeTextureFromResourceW");
	p[117] = GetProcAddress(hL, "D3DXDebugMute");
	p[118] = GetProcAddress(hL, "D3DXDeclaratorFromFVF");
	p[119] = GetProcAddress(hL, "D3DXDisassembleEffect");
	p[120] = GetProcAddress(hL, "D3DXDisassembleShader");
	p[121] = GetProcAddress(hL, "D3DXFVFFromDeclarator");
	p[122] = GetProcAddress(hL, "D3DXFileCreate");
	p[123] = GetProcAddress(hL, "D3DXFillCubeTexture");
	p[124] = GetProcAddress(hL, "D3DXFillCubeTextureTX");
	p[125] = GetProcAddress(hL, "D3DXFillTexture");
	p[126] = GetProcAddress(hL, "D3DXFillTextureTX");
	p[127] = GetProcAddress(hL, "D3DXFillVolumeTexture");
	p[128] = GetProcAddress(hL, "D3DXFillVolumeTextureTX");
	p[129] = GetProcAddress(hL, "D3DXFilterTexture");
	p[130] = GetProcAddress(hL, "D3DXFindShaderComment");
	p[131] = GetProcAddress(hL, "D3DXFloat16To32Array");
	p[132] = GetProcAddress(hL, "D3DXFloat32To16Array");
	p[133] = GetProcAddress(hL, "D3DXFrameAppendChild");
	p[134] = GetProcAddress(hL, "D3DXFrameCalculateBoundingSphere");
	p[135] = GetProcAddress(hL, "D3DXFrameDestroy");
	p[136] = GetProcAddress(hL, "D3DXFrameFind");
	p[137] = GetProcAddress(hL, "D3DXFrameNumNamedMatrices");
	p[138] = GetProcAddress(hL, "D3DXFrameRegisterNamedMatrices");
	p[139] = GetProcAddress(hL, "D3DXFresnelTerm");
	p[140] = GetProcAddress(hL, "D3DXGenerateOutputDecl");
	p[141] = GetProcAddress(hL, "D3DXGeneratePMesh");
	p[142] = GetProcAddress(hL, "D3DXGetDeclLength");
	p[143] = GetProcAddress(hL, "D3DXGetDeclVertexSize");
	p[144] = GetProcAddress(hL, "D3DXGetDriverLevel");
	p[145] = GetProcAddress(hL, "D3DXGetFVFVertexSize");
	p[146] = GetProcAddress(hL, "D3DXGetImageInfoFromFileA");
	p[147] = GetProcAddress(hL, "D3DXGetImageInfoFromFileInMemory");
	p[148] = GetProcAddress(hL, "D3DXGetImageInfoFromFileW");
	p[149] = GetProcAddress(hL, "D3DXGetImageInfoFromResourceA");
	p[150] = GetProcAddress(hL, "D3DXGetImageInfoFromResourceW");
	p[151] = GetProcAddress(hL, "D3DXGetPixelShaderProfile");
	p[152] = GetProcAddress(hL, "D3DXGetShaderConstantTable");
	p[153] = GetProcAddress(hL, "D3DXGetShaderConstantTableEx");
	p[154] = GetProcAddress(hL, "D3DXGetShaderInputSemantics");
	p[155] = GetProcAddress(hL, "D3DXGetShaderOutputSemantics");
	p[156] = GetProcAddress(hL, "D3DXGetShaderSamplers");
	p[157] = GetProcAddress(hL, "D3DXGetShaderSize");
	p[158] = GetProcAddress(hL, "D3DXGetShaderVersion");
	p[159] = GetProcAddress(hL, "D3DXGetVertexShaderProfile");
	p[160] = GetProcAddress(hL, "D3DXIntersect");
	p[161] = GetProcAddress(hL, "D3DXIntersectSubset");
	p[162] = GetProcAddress(hL, "D3DXIntersectTri");
	p[163] = GetProcAddress(hL, "D3DXLoadMeshFromXA");
	p[164] = GetProcAddress(hL, "D3DXLoadMeshFromXInMemory");
	p[165] = GetProcAddress(hL, "D3DXLoadMeshFromXResource");
	p[166] = GetProcAddress(hL, "D3DXLoadMeshFromXW");
	p[167] = GetProcAddress(hL, "D3DXLoadMeshFromXof");
	p[168] = GetProcAddress(hL, "D3DXLoadMeshHierarchyFromXA");
	p[169] = GetProcAddress(hL, "D3DXLoadMeshHierarchyFromXInMemory");
	p[170] = GetProcAddress(hL, "D3DXLoadMeshHierarchyFromXW");
	p[171] = GetProcAddress(hL, "D3DXLoadPRTBufferFromFileA");
	p[172] = GetProcAddress(hL, "D3DXLoadPRTBufferFromFileW");
	p[173] = GetProcAddress(hL, "D3DXLoadPRTCompBufferFromFileA");
	p[174] = GetProcAddress(hL, "D3DXLoadPRTCompBufferFromFileW");
	p[175] = GetProcAddress(hL, "D3DXLoadPatchMeshFromXof");
	p[176] = GetProcAddress(hL, "D3DXLoadSkinMeshFromXof");
	p[177] = GetProcAddress(hL, "D3DXLoadSurfaceFromFileA");
	p[178] = GetProcAddress(hL, "D3DXLoadSurfaceFromFileInMemory");
	p[179] = GetProcAddress(hL, "D3DXLoadSurfaceFromFileW");
	p[180] = GetProcAddress(hL, "D3DXLoadSurfaceFromMemory");
	p[181] = GetProcAddress(hL, "D3DXLoadSurfaceFromResourceA");
	p[182] = GetProcAddress(hL, "D3DXLoadSurfaceFromResourceW");
	p[183] = GetProcAddress(hL, "D3DXLoadSurfaceFromSurface");
	p[184] = GetProcAddress(hL, "D3DXLoadVolumeFromFileA");
	p[185] = GetProcAddress(hL, "D3DXLoadVolumeFromFileInMemory");
	p[186] = GetProcAddress(hL, "D3DXLoadVolumeFromFileW");
	p[187] = GetProcAddress(hL, "D3DXLoadVolumeFromMemory");
	p[188] = GetProcAddress(hL, "D3DXLoadVolumeFromResourceA");
	p[189] = GetProcAddress(hL, "D3DXLoadVolumeFromResourceW");
	p[190] = GetProcAddress(hL, "D3DXLoadVolumeFromVolume");
	p[191] = GetProcAddress(hL, "D3DXMatrixAffineTransformation");
	p[192] = GetProcAddress(hL, "D3DXMatrixAffineTransformation2D");
	p[193] = GetProcAddress(hL, "D3DXMatrixDecompose");
	p[194] = GetProcAddress(hL, "D3DXMatrixDeterminant");
	p[195] = GetProcAddress(hL, "D3DXMatrixInverse");
	p[196] = GetProcAddress(hL, "D3DXMatrixLookAtLH");
	p[197] = GetProcAddress(hL, "D3DXMatrixLookAtRH");
	p[198] = GetProcAddress(hL, "D3DXMatrixMultiply");
	p[199] = GetProcAddress(hL, "D3DXMatrixMultiplyTranspose");
	p[200] = GetProcAddress(hL, "D3DXMatrixOrthoLH");
	p[201] = GetProcAddress(hL, "D3DXMatrixOrthoOffCenterLH");
	p[202] = GetProcAddress(hL, "D3DXMatrixOrthoOffCenterRH");
	p[203] = GetProcAddress(hL, "D3DXMatrixOrthoRH");
	p[204] = GetProcAddress(hL, "D3DXMatrixPerspectiveFovLH");
	p[205] = GetProcAddress(hL, "D3DXMatrixPerspectiveFovRH");
	p[206] = GetProcAddress(hL, "D3DXMatrixPerspectiveLH");
	p[207] = GetProcAddress(hL, "D3DXMatrixPerspectiveOffCenterLH");
	p[208] = GetProcAddress(hL, "D3DXMatrixPerspectiveOffCenterRH");
	p[209] = GetProcAddress(hL, "D3DXMatrixPerspectiveRH");
	p[210] = GetProcAddress(hL, "D3DXMatrixReflect");
	p[211] = GetProcAddress(hL, "D3DXMatrixRotationAxis");
	p[212] = GetProcAddress(hL, "D3DXMatrixRotationQuaternion");
	p[213] = GetProcAddress(hL, "D3DXMatrixRotationX");
	p[214] = GetProcAddress(hL, "D3DXMatrixRotationY");
	p[215] = GetProcAddress(hL, "D3DXMatrixRotationYawPitchRoll");
	p[216] = GetProcAddress(hL, "D3DXMatrixRotationZ");
	p[217] = GetProcAddress(hL, "D3DXMatrixScaling");
	p[218] = GetProcAddress(hL, "D3DXMatrixShadow");
	p[219] = GetProcAddress(hL, "D3DXMatrixTransformation");
	p[220] = GetProcAddress(hL, "D3DXMatrixTransformation2D");
	p[221] = GetProcAddress(hL, "D3DXMatrixTranslation");
	p[222] = GetProcAddress(hL, "D3DXMatrixTranspose");
	p[223] = GetProcAddress(hL, "D3DXOptimizeFaces");
	p[224] = GetProcAddress(hL, "D3DXOptimizeVertices");
	p[225] = GetProcAddress(hL, "D3DXPlaneFromPointNormal");
	p[226] = GetProcAddress(hL, "D3DXPlaneFromPoints");
	p[227] = GetProcAddress(hL, "D3DXPlaneIntersectLine");
	p[228] = GetProcAddress(hL, "D3DXPlaneNormalize");
	p[229] = GetProcAddress(hL, "D3DXPlaneTransform");
	p[230] = GetProcAddress(hL, "D3DXPlaneTransformArray");
	p[231] = GetProcAddress(hL, "D3DXPreprocessShader");
	p[232] = GetProcAddress(hL, "D3DXPreprocessShaderFromFileA");
	p[233] = GetProcAddress(hL, "D3DXPreprocessShaderFromFileW");
	p[234] = GetProcAddress(hL, "D3DXPreprocessShaderFromResourceA");
	p[235] = GetProcAddress(hL, "D3DXPreprocessShaderFromResourceW");
	p[236] = GetProcAddress(hL, "D3DXQuaternionBaryCentric");
	p[237] = GetProcAddress(hL, "D3DXQuaternionExp");
	p[238] = GetProcAddress(hL, "D3DXQuaternionInverse");
	p[239] = GetProcAddress(hL, "D3DXQuaternionLn");
	p[240] = GetProcAddress(hL, "D3DXQuaternionMultiply");
	p[241] = GetProcAddress(hL, "D3DXQuaternionNormalize");
	p[242] = GetProcAddress(hL, "D3DXQuaternionRotationAxis");
	p[243] = GetProcAddress(hL, "D3DXQuaternionRotationMatrix");
	p[244] = GetProcAddress(hL, "D3DXQuaternionRotationYawPitchRoll");
	p[245] = GetProcAddress(hL, "D3DXQuaternionSlerp");
	p[246] = GetProcAddress(hL, "D3DXQuaternionSquad");
	p[247] = GetProcAddress(hL, "D3DXQuaternionSquadSetup");
	p[248] = GetProcAddress(hL, "D3DXQuaternionToAxisAngle");
	p[249] = GetProcAddress(hL, "D3DXRectPatchSize");
	p[250] = GetProcAddress(hL, "D3DXSHAdd");
	p[251] = GetProcAddress(hL, "D3DXSHDot");
	p[252] = GetProcAddress(hL, "D3DXSHEvalConeLight");
	p[253] = GetProcAddress(hL, "D3DXSHEvalDirection");
	p[254] = GetProcAddress(hL, "D3DXSHEvalDirectionalLight");
	p[255] = GetProcAddress(hL, "D3DXSHEvalHemisphereLight");
	p[256] = GetProcAddress(hL, "D3DXSHEvalSphericalLight");
	p[257] = GetProcAddress(hL, "D3DXSHMultiply2");
	p[258] = GetProcAddress(hL, "D3DXSHMultiply3");
	p[259] = GetProcAddress(hL, "D3DXSHMultiply4");
	p[260] = GetProcAddress(hL, "D3DXSHMultiply5");
	p[261] = GetProcAddress(hL, "D3DXSHMultiply6");
	p[262] = GetProcAddress(hL, "D3DXSHPRTCompSplitMeshSC");
	p[263] = GetProcAddress(hL, "D3DXSHPRTCompSuperCluster");
	p[264] = GetProcAddress(hL, "D3DXSHProjectCubeMap");
	p[265] = GetProcAddress(hL, "D3DXSHRotate");
	p[266] = GetProcAddress(hL, "D3DXSHRotateZ");
	p[267] = GetProcAddress(hL, "D3DXSHScale");
	p[268] = GetProcAddress(hL, "D3DXSaveMeshHierarchyToFileA");
	p[269] = GetProcAddress(hL, "D3DXSaveMeshHierarchyToFileW");
	p[270] = GetProcAddress(hL, "D3DXSaveMeshToXA");
	p[271] = GetProcAddress(hL, "D3DXSaveMeshToXW");
	p[272] = GetProcAddress(hL, "D3DXSavePRTBufferToFileA");
	p[273] = GetProcAddress(hL, "D3DXSavePRTBufferToFileW");
	p[274] = GetProcAddress(hL, "D3DXSavePRTCompBufferToFileA");
	p[275] = GetProcAddress(hL, "D3DXSavePRTCompBufferToFileW");
	p[276] = GetProcAddress(hL, "D3DXSaveSurfaceToFileA");
	p[277] = GetProcAddress(hL, "D3DXSaveSurfaceToFileInMemory");
	p[278] = GetProcAddress(hL, "D3DXSaveSurfaceToFileW");
	p[279] = GetProcAddress(hL, "D3DXSaveTextureToFileA");
	p[280] = GetProcAddress(hL, "D3DXSaveTextureToFileInMemory");
	p[281] = GetProcAddress(hL, "D3DXSaveTextureToFileW");
	p[282] = GetProcAddress(hL, "D3DXSaveVolumeToFileA");
	p[283] = GetProcAddress(hL, "D3DXSaveVolumeToFileInMemory");
	p[284] = GetProcAddress(hL, "D3DXSaveVolumeToFileW");
	p[285] = GetProcAddress(hL, "D3DXSimplifyMesh");
	p[286] = GetProcAddress(hL, "D3DXSphereBoundProbe");
	p[287] = GetProcAddress(hL, "D3DXSplitMesh");
	p[288] = GetProcAddress(hL, "D3DXTessellateNPatches");
	p[289] = GetProcAddress(hL, "D3DXTessellateRectPatch");
	p[290] = GetProcAddress(hL, "D3DXTessellateTriPatch");
	p[291] = GetProcAddress(hL, "D3DXTriPatchSize");
	p[292] = GetProcAddress(hL, "D3DXUVAtlasCreate");
	p[293] = GetProcAddress(hL, "D3DXUVAtlasPack");
	p[294] = GetProcAddress(hL, "D3DXUVAtlasPartition");
	p[295] = GetProcAddress(hL, "D3DXValidMesh");
	p[296] = GetProcAddress(hL, "D3DXValidPatchMesh");
	p[297] = GetProcAddress(hL, "D3DXVec2BaryCentric");
	p[298] = GetProcAddress(hL, "D3DXVec2CatmullRom");
	p[299] = GetProcAddress(hL, "D3DXVec2Hermite");
	p[300] = GetProcAddress(hL, "D3DXVec2Normalize");
	p[301] = GetProcAddress(hL, "D3DXVec2Transform");
	p[302] = GetProcAddress(hL, "D3DXVec2TransformArray");
	p[303] = GetProcAddress(hL, "D3DXVec2TransformCoord");
	p[304] = GetProcAddress(hL, "D3DXVec2TransformCoordArray");
	p[305] = GetProcAddress(hL, "D3DXVec2TransformNormal");
	p[306] = GetProcAddress(hL, "D3DXVec2TransformNormalArray");
	p[307] = GetProcAddress(hL, "D3DXVec3BaryCentric");
	p[308] = GetProcAddress(hL, "D3DXVec3CatmullRom");
	p[309] = GetProcAddress(hL, "D3DXVec3Hermite");
	p[310] = GetProcAddress(hL, "D3DXVec3Normalize");
	p[311] = GetProcAddress(hL, "D3DXVec3Project");
	p[312] = GetProcAddress(hL, "D3DXVec3ProjectArray");
	p[313] = GetProcAddress(hL, "D3DXVec3Transform");
	p[314] = GetProcAddress(hL, "D3DXVec3TransformArray");
	p[315] = GetProcAddress(hL, "D3DXVec3TransformCoord");
	p[316] = GetProcAddress(hL, "D3DXVec3TransformCoordArray");
	p[317] = GetProcAddress(hL, "D3DXVec3TransformNormal");
	p[318] = GetProcAddress(hL, "D3DXVec3TransformNormalArray");
	p[319] = GetProcAddress(hL, "D3DXVec3Unproject");
	p[320] = GetProcAddress(hL, "D3DXVec3UnprojectArray");
	p[321] = GetProcAddress(hL, "D3DXVec4BaryCentric");
	p[322] = GetProcAddress(hL, "D3DXVec4CatmullRom");
	p[323] = GetProcAddress(hL, "D3DXVec4Cross");
	p[324] = GetProcAddress(hL, "D3DXVec4Hermite");
	p[325] = GetProcAddress(hL, "D3DXVec4Normalize");
	p[326] = GetProcAddress(hL, "D3DXVec4Transform");
	p[327] = GetProcAddress(hL, "D3DXVec4TransformArray");
	p[328] = GetProcAddress(hL, "D3DXWeldVertices");

	return true;
}

// D3DXAssembleShader
extern "C" __declspec(naked) void __cdecl __E__0__() {
	__asm
	{
		jmp p[0 * 4];
	}
}

// D3DXAssembleShaderFromFileA
extern "C" __declspec(naked) void __cdecl __E__1__() {
	__asm
	{
		jmp p[1 * 4];
	}
}

// D3DXAssembleShaderFromFileW
extern "C" __declspec(naked) void __cdecl __E__2__() {
	__asm
	{
		jmp p[2 * 4];
	}
}

// D3DXAssembleShaderFromResourceA
extern "C" __declspec(naked) void __cdecl __E__3__() {
	__asm
	{
		jmp p[3 * 4];
	}
}

// D3DXAssembleShaderFromResourceW
extern "C" __declspec(naked) void __cdecl __E__4__() {
	__asm
	{
		jmp p[4 * 4];
	}
}

// D3DXBoxBoundProbe
extern "C" __declspec(naked) void __cdecl __E__5__() {
	__asm
	{
		jmp p[5 * 4];
	}
}

// D3DXCheckCubeTextureRequirements
extern "C" __declspec(naked) void __cdecl __E__6__() {
	__asm
	{
		jmp p[6 * 4];
	}
}

// D3DXCheckTextureRequirements
extern "C" __declspec(naked) void __cdecl __E__7__() {
	__asm
	{
		jmp p[7 * 4];
	}
}

// D3DXCheckVersion
extern "C" __declspec(naked) void __cdecl __E__8__() {
	__asm
	{
		jmp p[8 * 4];
	}
}

// D3DXCheckVolumeTextureRequirements
extern "C" __declspec(naked) void __cdecl __E__9__() {
	__asm
	{
		jmp p[9 * 4];
	}
}

// D3DXCleanMesh
extern "C" __declspec(naked) void __cdecl __E__10__() {
	__asm
	{
		jmp p[10 * 4];
	}
}

// D3DXColorAdjustContrast
extern "C" __declspec(naked) void __cdecl __E__11__() {
	__asm
	{
		jmp p[11 * 4];
	}
}

// D3DXColorAdjustSaturation
extern "C" __declspec(naked) void __cdecl __E__12__() {
	__asm
	{
		jmp p[12 * 4];
	}
}

// D3DXCompileShader
extern "C" __declspec(naked) void __cdecl __E__13__() {
	__asm
	{
		jmp p[13 * 4];
	}
}

// D3DXCompileShaderFromFileA
extern "C" __declspec(naked) void __cdecl __E__14__() {
	__asm
	{
		jmp p[14 * 4];
	}
}

// D3DXCompileShaderFromFileW
extern "C" __declspec(naked) void __cdecl __E__15__() {
	__asm
	{
		jmp p[15 * 4];
	}
}

// D3DXCompileShaderFromResourceA
extern "C" __declspec(naked) void __cdecl __E__16__() {
	__asm
	{
		jmp p[16 * 4];
	}
}

// D3DXCompileShaderFromResourceW
extern "C" __declspec(naked) void __cdecl __E__17__() {
	__asm
	{
		jmp p[17 * 4];
	}
}

// D3DXComputeBoundingBox
extern "C" __declspec(naked) void __cdecl __E__18__() {
	__asm
	{
		jmp p[18 * 4];
	}
}

// D3DXComputeBoundingSphere
extern "C" __declspec(naked) void __cdecl __E__19__() {
	__asm
	{
		jmp p[19 * 4];
	}
}

// D3DXComputeIMTFromPerTexelSignal
extern "C" __declspec(naked) void __cdecl __E__20__() {
	__asm
	{
		jmp p[20 * 4];
	}
}

// D3DXComputeIMTFromPerVertexSignal
extern "C" __declspec(naked) void __cdecl __E__21__() {
	__asm
	{
		jmp p[21 * 4];
	}
}

// D3DXComputeIMTFromSignal
extern "C" __declspec(naked) void __cdecl __E__22__() {
	__asm
	{
		jmp p[22 * 4];
	}
}

// D3DXComputeIMTFromTexture
extern "C" __declspec(naked) void __cdecl __E__23__() {
	__asm
	{
		jmp p[23 * 4];
	}
}

// D3DXComputeNormalMap
extern "C" __declspec(naked) void __cdecl __E__24__() {
	__asm
	{
		jmp p[24 * 4];
	}
}

// D3DXComputeNormals
extern "C" __declspec(naked) void __cdecl __E__25__() {
	__asm
	{
		jmp p[25 * 4];
	}
}

// D3DXComputeTangent
extern "C" __declspec(naked) void __cdecl __E__26__() {
	__asm
	{
		jmp p[26 * 4];
	}
}

// D3DXComputeTangentFrame
extern "C" __declspec(naked) void __cdecl __E__27__() {
	__asm
	{
		jmp p[27 * 4];
	}
}

// D3DXComputeTangentFrameEx
extern "C" __declspec(naked) void __cdecl __E__28__() {
	__asm
	{
		jmp p[28 * 4];
	}
}

// D3DXConcatenateMeshes
extern "C" __declspec(naked) void __cdecl __E__29__() {
	__asm
	{
		jmp p[29 * 4];
	}
}

// D3DXConvertMeshSubsetToSingleStrip
extern "C" __declspec(naked) void __cdecl __E__30__() {
	__asm
	{
		jmp p[30 * 4];
	}
}

// D3DXConvertMeshSubsetToStrips
extern "C" __declspec(naked) void __cdecl __E__31__() {
	__asm
	{
		jmp p[31 * 4];
	}
}

// D3DXCreateAnimationController
extern "C" __declspec(naked) void __cdecl __E__32__() {
	__asm
	{
		jmp p[32 * 4];
	}
}

// D3DXCreateBox
extern "C" __declspec(naked) void __cdecl __E__33__() {
	__asm
	{
		jmp p[33 * 4];
	}
}

// D3DXCreateBuffer
extern "C" __declspec(naked) void __cdecl __E__34__() {
	__asm
	{
		jmp p[34 * 4];
	}
}

// D3DXCreateCompressedAnimationSet
extern "C" __declspec(naked) void __cdecl __E__35__() {
	__asm
	{
		jmp p[35 * 4];
	}
}

// D3DXCreateCubeTexture
extern "C" __declspec(naked) void __cdecl __E__36__() {
	__asm
	{
		jmp p[36 * 4];
	}
}

// D3DXCreateCubeTextureFromFileA
extern "C" __declspec(naked) void __cdecl __E__37__() {
	__asm
	{
		jmp p[37 * 4];
	}
}

// D3DXCreateCubeTextureFromFileExA
extern "C" __declspec(naked) void __cdecl __E__38__() {
	__asm
	{
		jmp p[38 * 4];
	}
}

// D3DXCreateCubeTextureFromFileExW
extern "C" __declspec(naked) void __cdecl __E__39__() {
	__asm
	{
		jmp p[39 * 4];
	}
}

// D3DXCreateCubeTextureFromFileInMemory
extern "C" __declspec(naked) void __cdecl __E__40__() {
	__asm
	{
		jmp p[40 * 4];
	}
}

// D3DXCreateCubeTextureFromFileInMemoryEx
extern "C" __declspec(naked) void __cdecl __E__41__() {
	__asm
	{
		jmp p[41 * 4];
	}
}

// D3DXCreateCubeTextureFromFileW
extern "C" __declspec(naked) void __cdecl __E__42__() {
	__asm
	{
		jmp p[42 * 4];
	}
}

// D3DXCreateCubeTextureFromResourceA
extern "C" __declspec(naked) void __cdecl __E__43__() {
	__asm
	{
		jmp p[43 * 4];
	}
}

// D3DXCreateCubeTextureFromResourceExA
extern "C" __declspec(naked) void __cdecl __E__44__() {
	__asm
	{
		jmp p[44 * 4];
	}
}

// D3DXCreateCubeTextureFromResourceExW
extern "C" __declspec(naked) void __cdecl __E__45__() {
	__asm
	{
		jmp p[45 * 4];
	}
}

// D3DXCreateCubeTextureFromResourceW
extern "C" __declspec(naked) void __cdecl __E__46__() {
	__asm
	{
		jmp p[46 * 4];
	}
}

// D3DXCreateCylinder
extern "C" __declspec(naked) void __cdecl __E__47__() {
	__asm
	{
		jmp p[47 * 4];
	}
}

// D3DXCreateEffect
extern "C" __declspec(naked) void __cdecl __E__48__() {
	__asm
	{
		jmp p[48 * 4];
	}
}

// D3DXCreateEffectCompiler
extern "C" __declspec(naked) void __cdecl __E__49__() {
	__asm
	{
		jmp p[49 * 4];
	}
}

// D3DXCreateEffectCompilerFromFileA
extern "C" __declspec(naked) void __cdecl __E__50__() {
	__asm
	{
		jmp p[50 * 4];
	}
}

// D3DXCreateEffectCompilerFromFileW
extern "C" __declspec(naked) void __cdecl __E__51__() {
	__asm
	{
		jmp p[51 * 4];
	}
}

// D3DXCreateEffectCompilerFromResourceA
extern "C" __declspec(naked) void __cdecl __E__52__() {
	__asm
	{
		jmp p[52 * 4];
	}
}

// D3DXCreateEffectCompilerFromResourceW
extern "C" __declspec(naked) void __cdecl __E__53__() {
	__asm
	{
		jmp p[53 * 4];
	}
}

// D3DXCreateEffectEx
extern "C" __declspec(naked) void __cdecl __E__54__() {
	__asm
	{
		jmp p[54 * 4];
	}
}

// D3DXCreateEffectFromFileA
extern "C" __declspec(naked) void __cdecl __E__55__() {
	__asm
	{
		jmp p[55 * 4];
	}
}

// D3DXCreateEffectFromFileExA
extern "C" __declspec(naked) void __cdecl __E__56__() {
	__asm
	{
		jmp p[56 * 4];
	}
}

// D3DXCreateEffectFromFileExW
extern "C" __declspec(naked) void __cdecl __E__57__() {
	__asm
	{
		jmp p[57 * 4];
	}
}

// D3DXCreateEffectFromFileW
extern "C" __declspec(naked) void __cdecl __E__58__() {
	__asm
	{
		jmp p[58 * 4];
	}
}

// D3DXCreateEffectFromResourceA
extern "C" __declspec(naked) void __cdecl __E__59__() {
	__asm
	{
		jmp p[59 * 4];
	}
}

// D3DXCreateEffectFromResourceExA
extern "C" __declspec(naked) void __cdecl __E__60__() {
	__asm
	{
		jmp p[60 * 4];
	}
}

// D3DXCreateEffectFromResourceExW
extern "C" __declspec(naked) void __cdecl __E__61__() {
	__asm
	{
		jmp p[61 * 4];
	}
}

// D3DXCreateEffectFromResourceW
extern "C" __declspec(naked) void __cdecl __E__62__() {
	__asm
	{
		jmp p[62 * 4];
	}
}

// D3DXCreateEffectPool
extern "C" __declspec(naked) void __cdecl __E__63__() {
	__asm
	{
		jmp p[63 * 4];
	}
}

// D3DXCreateFontA
extern "C" __declspec(naked) void __cdecl __E__64__() {
	__asm
	{
		jmp p[64 * 4];
	}
}

// D3DXCreateFontIndirectA
extern "C" __declspec(naked) void __cdecl __E__65__() {
	__asm
	{
		jmp p[65 * 4];
	}
}

// D3DXCreateFontIndirectW
extern "C" __declspec(naked) void __cdecl __E__66__() {
	__asm
	{
		jmp p[66 * 4];
	}
}

// D3DXCreateFontW
extern "C" __declspec(naked) void __cdecl __E__67__() {
	__asm
	{
		jmp p[67 * 4];
	}
}

// D3DXCreateKeyframedAnimationSet
extern "C" __declspec(naked) void __cdecl __E__68__() {
	__asm
	{
		jmp p[68 * 4];
	}
}

// D3DXCreateLine
extern "C" __declspec(naked) void __cdecl __E__69__() {
	__asm
	{
		jmp p[69 * 4];
	}
}

// D3DXCreateMatrixStack
extern "C" __declspec(naked) void __cdecl __E__70__() {
	__asm
	{
		jmp p[70 * 4];
	}
}

// D3DXCreateMesh
extern "C" __declspec(naked) void __cdecl __E__71__() {
	__asm
	{
		jmp p[71 * 4];
	}
}

// D3DXCreateMeshFVF
extern "C" __declspec(naked) void __cdecl __E__72__() {
	__asm
	{
		jmp p[72 * 4];
	}
}

// D3DXCreateNPatchMesh
extern "C" __declspec(naked) void __cdecl __E__73__() {
	__asm
	{
		jmp p[73 * 4];
	}
}

// D3DXCreatePMeshFromStream
extern "C" __declspec(naked) void __cdecl __E__74__() {
	__asm
	{
		jmp p[74 * 4];
	}
}

// D3DXCreatePRTBuffer
extern "C" __declspec(naked) void __cdecl __E__75__() {
	__asm
	{
		jmp p[75 * 4];
	}
}

// D3DXCreatePRTBufferTex
extern "C" __declspec(naked) void __cdecl __E__76__() {
	__asm
	{
		jmp p[76 * 4];
	}
}

// D3DXCreatePRTCompBuffer
extern "C" __declspec(naked) void __cdecl __E__77__() {
	__asm
	{
		jmp p[77 * 4];
	}
}

// D3DXCreatePRTEngine
extern "C" __declspec(naked) void __cdecl __E__78__() {
	__asm
	{
		jmp p[78 * 4];
	}
}

// D3DXCreatePatchMesh
extern "C" __declspec(naked) void __cdecl __E__79__() {
	__asm
	{
		jmp p[79 * 4];
	}
}

// D3DXCreatePolygon
extern "C" __declspec(naked) void __cdecl __E__80__() {
	__asm
	{
		jmp p[80 * 4];
	}
}

// D3DXCreateRenderToEnvMap
extern "C" __declspec(naked) void __cdecl __E__81__() {
	__asm
	{
		jmp p[81 * 4];
	}
}

// D3DXCreateRenderToSurface
extern "C" __declspec(naked) void __cdecl __E__82__() {
	__asm
	{
		jmp p[82 * 4];
	}
}

// D3DXCreateSPMesh
extern "C" __declspec(naked) void __cdecl __E__83__() {
	__asm
	{
		jmp p[83 * 4];
	}
}

// D3DXCreateSkinInfo
extern "C" __declspec(naked) void __cdecl __E__84__() {
	__asm
	{
		jmp p[84 * 4];
	}
}

// D3DXCreateSkinInfoFVF
extern "C" __declspec(naked) void __cdecl __E__85__() {
	__asm
	{
		jmp p[85 * 4];
	}
}

// D3DXCreateSkinInfoFromBlendedMesh
extern "C" __declspec(naked) void __cdecl __E__86__() {
	__asm
	{
		jmp p[86 * 4];
	}
}

// D3DXCreateSphere
extern "C" __declspec(naked) void __cdecl __E__87__() {
	__asm
	{
		jmp p[87 * 4];
	}
}

// D3DXCreateSprite
extern "C" __declspec(naked) void __cdecl __E__88__() {
	__asm
	{
		jmp p[88 * 4];
	}
}

// D3DXCreateTeapot
extern "C" __declspec(naked) void __cdecl __E__89__() {
	__asm
	{
		jmp p[89 * 4];
	}
}

// D3DXCreateTextA
extern "C" __declspec(naked) void __cdecl __E__90__() {
	__asm
	{
		jmp p[90 * 4];
	}
}

// D3DXCreateTextW
extern "C" __declspec(naked) void __cdecl __E__91__() {
	__asm
	{
		jmp p[91 * 4];
	}
}

// D3DXCreateTexture
extern "C" __declspec(naked) void __cdecl __E__92__() {
	__asm
	{
		jmp p[92 * 4];
	}
}

// D3DXCreateTextureFromFileA
extern "C" __declspec(naked) void __cdecl __E__93__() {
	__asm
	{
		jmp p[93 * 4];
	}
}

// D3DXCreateTextureFromFileExA
extern "C" __declspec(naked) void __cdecl __E__94__() {
	__asm
	{
		jmp p[94 * 4];
	}
}

// D3DXCreateTextureFromFileExW
extern "C" __declspec(naked) void __cdecl __E__95__() {
	__asm
	{
		jmp p[95 * 4];
	}
}

// D3DXCreateTextureFromFileInMemory
extern "C" __declspec(naked) void __cdecl __E__96__() {
	__asm
	{
		jmp p[96 * 4];
	}
}

// D3DXCreateTextureFromFileInMemoryEx
extern "C" __declspec(naked) void __cdecl __E__97__() {
	__asm
	{
		jmp p[97 * 4];
	}
}

// D3DXCreateTextureFromFileW
extern "C" __declspec(naked) void __cdecl __E__98__() {
	__asm
	{
		jmp p[98 * 4];
	}
}

// D3DXCreateTextureFromResourceA
extern "C" __declspec(naked) void __cdecl __E__99__() {
	__asm
	{
		jmp p[99 * 4];
	}
}

// D3DXCreateTextureFromResourceExA
extern "C" __declspec(naked) void __cdecl __E__100__() {
	__asm
	{
		jmp p[100 * 4];
	}
}

// D3DXCreateTextureFromResourceExW
extern "C" __declspec(naked) void __cdecl __E__101__() {
	__asm
	{
		jmp p[101 * 4];
	}
}

// D3DXCreateTextureFromResourceW
extern "C" __declspec(naked) void __cdecl __E__102__() {
	__asm
	{
		jmp p[102 * 4];
	}
}

// D3DXCreateTextureGutterHelper
extern "C" __declspec(naked) void __cdecl __E__103__() {
	__asm
	{
		jmp p[103 * 4];
	}
}

// D3DXCreateTextureShader
extern "C" __declspec(naked) void __cdecl __E__104__() {
	__asm
	{
		jmp p[104 * 4];
	}
}

// D3DXCreateTorus
extern "C" __declspec(naked) void __cdecl __E__105__() {
	__asm
	{
		jmp p[105 * 4];
	}
}

// D3DXCreateVolumeTexture
extern "C" __declspec(naked) void __cdecl __E__106__() {
	__asm
	{
		jmp p[106 * 4];
	}
}

// D3DXCreateVolumeTextureFromFileA
extern "C" __declspec(naked) void __cdecl __E__107__() {
	__asm
	{
		jmp p[107 * 4];
	}
}

// D3DXCreateVolumeTextureFromFileExA
extern "C" __declspec(naked) void __cdecl __E__108__() {
	__asm
	{
		jmp p[108 * 4];
	}
}

// D3DXCreateVolumeTextureFromFileExW
extern "C" __declspec(naked) void __cdecl __E__109__() {
	__asm
	{
		jmp p[109 * 4];
	}
}

// D3DXCreateVolumeTextureFromFileInMemory
extern "C" __declspec(naked) void __cdecl __E__110__() {
	__asm
	{
		jmp p[110 * 4];
	}
}

// D3DXCreateVolumeTextureFromFileInMemoryEx
extern "C" __declspec(naked) void __cdecl __E__111__() {
	__asm
	{
		jmp p[111 * 4];
	}
}

// D3DXCreateVolumeTextureFromFileW
extern "C" __declspec(naked) void __cdecl __E__112__() {
	__asm
	{
		jmp p[112 * 4];
	}
}

// D3DXCreateVolumeTextureFromResourceA
extern "C" __declspec(naked) void __cdecl __E__113__() {
	__asm
	{
		jmp p[113 * 4];
	}
}

// D3DXCreateVolumeTextureFromResourceExA
extern "C" __declspec(naked) void __cdecl __E__114__() {
	__asm
	{
		jmp p[114 * 4];
	}
}

// D3DXCreateVolumeTextureFromResourceExW
extern "C" __declspec(naked) void __cdecl __E__115__() {
	__asm
	{
		jmp p[115 * 4];
	}
}

// D3DXCreateVolumeTextureFromResourceW
extern "C" __declspec(naked) void __cdecl __E__116__() {
	__asm
	{
		jmp p[116 * 4];
	}
}

// D3DXDebugMute
extern "C" __declspec(naked) void __cdecl __E__117__() {
	__asm
	{
		jmp p[117 * 4];
	}
}

// D3DXDeclaratorFromFVF
extern "C" __declspec(naked) void __cdecl __E__118__() {
	__asm
	{
		jmp p[118 * 4];
	}
}

// D3DXDisassembleEffect
extern "C" __declspec(naked) void __cdecl __E__119__() {
	__asm
	{
		jmp p[119 * 4];
	}
}

// D3DXDisassembleShader
extern "C" __declspec(naked) void __cdecl __E__120__() {
	__asm
	{
		jmp p[120 * 4];
	}
}

// D3DXFVFFromDeclarator
extern "C" __declspec(naked) void __cdecl __E__121__() {
	__asm
	{
		jmp p[121 * 4];
	}
}

// D3DXFileCreate
extern "C" __declspec(naked) void __cdecl __E__122__() {
	__asm
	{
		jmp p[122 * 4];
	}
}

// D3DXFillCubeTexture
extern "C" __declspec(naked) void __cdecl __E__123__() {
	__asm
	{
		jmp p[123 * 4];
	}
}

// D3DXFillCubeTextureTX
extern "C" __declspec(naked) void __cdecl __E__124__() {
	__asm
	{
		jmp p[124 * 4];
	}
}

// D3DXFillTexture
extern "C" __declspec(naked) void __cdecl __E__125__() {
	__asm
	{
		jmp p[125 * 4];
	}
}

// D3DXFillTextureTX
extern "C" __declspec(naked) void __cdecl __E__126__() {
	__asm
	{
		jmp p[126 * 4];
	}
}

// D3DXFillVolumeTexture
extern "C" __declspec(naked) void __cdecl __E__127__() {
	__asm
	{
		jmp p[127 * 4];
	}
}

// D3DXFillVolumeTextureTX
extern "C" __declspec(naked) void __cdecl __E__128__() {
	__asm
	{
		jmp p[128 * 4];
	}
}

// D3DXFilterTexture
extern "C" __declspec(naked) void __cdecl __E__129__() {
	__asm
	{
		jmp p[129 * 4];
	}
}

// D3DXFindShaderComment
extern "C" __declspec(naked) void __cdecl __E__130__() {
	__asm
	{
		jmp p[130 * 4];
	}
}

// D3DXFloat16To32Array
extern "C" __declspec(naked) void __cdecl __E__131__() {
	__asm
	{
		jmp p[131 * 4];
	}
}

// D3DXFloat32To16Array
extern "C" __declspec(naked) void __cdecl __E__132__() {
	__asm
	{
		jmp p[132 * 4];
	}
}

// D3DXFrameAppendChild
extern "C" __declspec(naked) void __cdecl __E__133__() {
	__asm
	{
		jmp p[133 * 4];
	}
}

// D3DXFrameCalculateBoundingSphere
extern "C" __declspec(naked) void __cdecl __E__134__() {
	__asm
	{
		jmp p[134 * 4];
	}
}

// D3DXFrameDestroy
extern "C" __declspec(naked) void __cdecl __E__135__() {
	__asm
	{
		jmp p[135 * 4];
	}
}

// D3DXFrameFind
extern "C" __declspec(naked) void __cdecl __E__136__() {
	__asm
	{
		jmp p[136 * 4];
	}
}

// D3DXFrameNumNamedMatrices
extern "C" __declspec(naked) void __cdecl __E__137__() {
	__asm
	{
		jmp p[137 * 4];
	}
}

// D3DXFrameRegisterNamedMatrices
extern "C" __declspec(naked) void __cdecl __E__138__() {
	__asm
	{
		jmp p[138 * 4];
	}
}

// D3DXFresnelTerm
extern "C" __declspec(naked) void __cdecl __E__139__() {
	__asm
	{
		jmp p[139 * 4];
	}
}

// D3DXGenerateOutputDecl
extern "C" __declspec(naked) void __cdecl __E__140__() {
	__asm
	{
		jmp p[140 * 4];
	}
}

// D3DXGeneratePMesh
extern "C" __declspec(naked) void __cdecl __E__141__() {
	__asm
	{
		jmp p[141 * 4];
	}
}

// D3DXGetDeclLength
extern "C" __declspec(naked) void __cdecl __E__142__() {
	__asm
	{
		jmp p[142 * 4];
	}
}

// D3DXGetDeclVertexSize
extern "C" __declspec(naked) void __cdecl __E__143__() {
	__asm
	{
		jmp p[143 * 4];
	}
}

// D3DXGetDriverLevel
extern "C" __declspec(naked) void __cdecl __E__144__() {
	__asm
	{
		jmp p[144 * 4];
	}
}

// D3DXGetFVFVertexSize
extern "C" __declspec(naked) void __cdecl __E__145__() {
	__asm
	{
		jmp p[145 * 4];
	}
}

// D3DXGetImageInfoFromFileA
extern "C" __declspec(naked) void __cdecl __E__146__() {
	__asm
	{
		jmp p[146 * 4];
	}
}

// D3DXGetImageInfoFromFileInMemory
extern "C" __declspec(naked) void __cdecl __E__147__() {
	__asm
	{
		jmp p[147 * 4];
	}
}

// D3DXGetImageInfoFromFileW
extern "C" __declspec(naked) void __cdecl __E__148__() {
	__asm
	{
		jmp p[148 * 4];
	}
}

// D3DXGetImageInfoFromResourceA
extern "C" __declspec(naked) void __cdecl __E__149__() {
	__asm
	{
		jmp p[149 * 4];
	}
}

// D3DXGetImageInfoFromResourceW
extern "C" __declspec(naked) void __cdecl __E__150__() {
	__asm
	{
		jmp p[150 * 4];
	}
}

// D3DXGetPixelShaderProfile
extern "C" __declspec(naked) void __cdecl __E__151__() {
	__asm
	{
		jmp p[151 * 4];
	}
}

// D3DXGetShaderConstantTable
extern "C" __declspec(naked) void __cdecl __E__152__() {
	__asm
	{
		jmp p[152 * 4];
	}
}

// D3DXGetShaderConstantTableEx
extern "C" __declspec(naked) void __cdecl __E__153__() {
	__asm
	{
		jmp p[153 * 4];
	}
}

// D3DXGetShaderInputSemantics
extern "C" __declspec(naked) void __cdecl __E__154__() {
	__asm
	{
		jmp p[154 * 4];
	}
}

// D3DXGetShaderOutputSemantics
extern "C" __declspec(naked) void __cdecl __E__155__() {
	__asm
	{
		jmp p[155 * 4];
	}
}

// D3DXGetShaderSamplers
extern "C" __declspec(naked) void __cdecl __E__156__() {
	__asm
	{
		jmp p[156 * 4];
	}
}

// D3DXGetShaderSize
extern "C" __declspec(naked) void __cdecl __E__157__() {
	__asm
	{
		jmp p[157 * 4];
	}
}

// D3DXGetShaderVersion
extern "C" __declspec(naked) void __cdecl __E__158__() {
	__asm
	{
		jmp p[158 * 4];
	}
}

// D3DXGetVertexShaderProfile
extern "C" __declspec(naked) void __cdecl __E__159__() {
	__asm
	{
		jmp p[159 * 4];
	}
}

// D3DXIntersect
extern "C" __declspec(naked) void __cdecl __E__160__() {
	__asm
	{
		jmp p[160 * 4];
	}
}

// D3DXIntersectSubset
extern "C" __declspec(naked) void __cdecl __E__161__() {
	__asm
	{
		jmp p[161 * 4];
	}
}

// D3DXIntersectTri
extern "C" __declspec(naked) void __cdecl __E__162__() {
	__asm
	{
		jmp p[162 * 4];
	}
}

// D3DXLoadMeshFromXA
extern "C" __declspec(naked) void __cdecl __E__163__() {
	__asm
	{
		jmp p[163 * 4];
	}
}

// D3DXLoadMeshFromXInMemory
extern "C" __declspec(naked) void __cdecl __E__164__() {
	__asm
	{
		jmp p[164 * 4];
	}
}

// D3DXLoadMeshFromXResource
extern "C" __declspec(naked) void __cdecl __E__165__() {
	__asm
	{
		jmp p[165 * 4];
	}
}

// D3DXLoadMeshFromXW
extern "C" __declspec(naked) void __cdecl __E__166__() {
	__asm
	{
		jmp p[166 * 4];
	}
}

// D3DXLoadMeshFromXof
extern "C" __declspec(naked) void __cdecl __E__167__() {
	__asm
	{
		jmp p[167 * 4];
	}
}

// D3DXLoadMeshHierarchyFromXA
extern "C" __declspec(naked) void __cdecl __E__168__() {
	__asm
	{
		jmp p[168 * 4];
	}
}

// D3DXLoadMeshHierarchyFromXInMemory
extern "C" __declspec(naked) void __cdecl __E__169__() {
	__asm
	{
		jmp p[169 * 4];
	}
}

// D3DXLoadMeshHierarchyFromXW
extern "C" __declspec(naked) void __cdecl __E__170__() {
	__asm
	{
		jmp p[170 * 4];
	}
}

// D3DXLoadPRTBufferFromFileA
extern "C" __declspec(naked) void __cdecl __E__171__() {
	__asm
	{
		jmp p[171 * 4];
	}
}

// D3DXLoadPRTBufferFromFileW
extern "C" __declspec(naked) void __cdecl __E__172__() {
	__asm
	{
		jmp p[172 * 4];
	}
}

// D3DXLoadPRTCompBufferFromFileA
extern "C" __declspec(naked) void __cdecl __E__173__() {
	__asm
	{
		jmp p[173 * 4];
	}
}

// D3DXLoadPRTCompBufferFromFileW
extern "C" __declspec(naked) void __cdecl __E__174__() {
	__asm
	{
		jmp p[174 * 4];
	}
}

// D3DXLoadPatchMeshFromXof
extern "C" __declspec(naked) void __cdecl __E__175__() {
	__asm
	{
		jmp p[175 * 4];
	}
}

// D3DXLoadSkinMeshFromXof
extern "C" __declspec(naked) void __cdecl __E__176__() {
	__asm
	{
		jmp p[176 * 4];
	}
}

// D3DXLoadSurfaceFromFileA
extern "C" __declspec(naked) void __cdecl __E__177__() {
	__asm
	{
		jmp p[177 * 4];
	}
}

// D3DXLoadSurfaceFromFileInMemory
extern "C" __declspec(naked) void __cdecl __E__178__() {
	__asm
	{
		jmp p[178 * 4];
	}
}

// D3DXLoadSurfaceFromFileW
extern "C" __declspec(naked) void __cdecl __E__179__() {
	__asm
	{
		jmp p[179 * 4];
	}
}

// D3DXLoadSurfaceFromMemory
extern "C" __declspec(naked) void __cdecl __E__180__() {
	__asm
	{
		jmp p[180 * 4];
	}
}

// D3DXLoadSurfaceFromResourceA
extern "C" __declspec(naked) void __cdecl __E__181__() {
	__asm
	{
		jmp p[181 * 4];
	}
}

// D3DXLoadSurfaceFromResourceW
extern "C" __declspec(naked) void __cdecl __E__182__() {
	__asm
	{
		jmp p[182 * 4];
	}
}

// D3DXLoadSurfaceFromSurface
extern "C" __declspec(naked) void __cdecl __E__183__() {
	__asm
	{
		jmp p[183 * 4];
	}
}

// D3DXLoadVolumeFromFileA
extern "C" __declspec(naked) void __cdecl __E__184__() {
	__asm
	{
		jmp p[184 * 4];
	}
}

// D3DXLoadVolumeFromFileInMemory
extern "C" __declspec(naked) void __cdecl __E__185__() {
	__asm
	{
		jmp p[185 * 4];
	}
}

// D3DXLoadVolumeFromFileW
extern "C" __declspec(naked) void __cdecl __E__186__() {
	__asm
	{
		jmp p[186 * 4];
	}
}

// D3DXLoadVolumeFromMemory
extern "C" __declspec(naked) void __cdecl __E__187__() {
	__asm
	{
		jmp p[187 * 4];
	}
}

// D3DXLoadVolumeFromResourceA
extern "C" __declspec(naked) void __cdecl __E__188__() {
	__asm
	{
		jmp p[188 * 4];
	}
}

// D3DXLoadVolumeFromResourceW
extern "C" __declspec(naked) void __cdecl __E__189__() {
	__asm
	{
		jmp p[189 * 4];
	}
}

// D3DXLoadVolumeFromVolume
extern "C" __declspec(naked) void __cdecl __E__190__() {
	__asm
	{
		jmp p[190 * 4];
	}
}

// D3DXMatrixAffineTransformation
extern "C" __declspec(naked) void __cdecl __E__191__() {
	__asm
	{
		jmp p[191 * 4];
	}
}

// D3DXMatrixAffineTransformation2D
extern "C" __declspec(naked) void __cdecl __E__192__() {
	__asm
	{
		jmp p[192 * 4];
	}
}

// D3DXMatrixDecompose
extern "C" __declspec(naked) void __cdecl __E__193__() {
	__asm
	{
		jmp p[193 * 4];
	}
}

// D3DXMatrixDeterminant
extern "C" __declspec(naked) void __cdecl __E__194__() {
	__asm
	{
		jmp p[194 * 4];
	}
}

// D3DXMatrixInverse
extern "C" __declspec(naked) void __cdecl __E__195__() {
	__asm
	{
		jmp p[195 * 4];
	}
}

// D3DXMatrixLookAtLH
extern "C" __declspec(naked) void __cdecl __E__196__() {
	__asm
	{
		jmp p[196 * 4];
	}
}

// D3DXMatrixLookAtRH
extern "C" __declspec(naked) void __cdecl __E__197__() {
	__asm
	{
		jmp p[197 * 4];
	}
}

// D3DXMatrixMultiply
extern "C" __declspec(naked) void __cdecl __E__198__() {
	__asm
	{
		jmp p[198 * 4];
	}
}

// D3DXMatrixMultiplyTranspose
extern "C" __declspec(naked) void __cdecl __E__199__() {
	__asm
	{
		jmp p[199 * 4];
	}
}

// D3DXMatrixOrthoLH
extern "C" __declspec(naked) void __cdecl __E__200__() {
	__asm
	{
		jmp p[200 * 4];
	}
}

// D3DXMatrixOrthoOffCenterLH
extern "C" __declspec(naked) void __cdecl __E__201__() {
	__asm
	{
		jmp p[201 * 4];
	}
}

// D3DXMatrixOrthoOffCenterRH
extern "C" __declspec(naked) void __cdecl __E__202__() {
	__asm
	{
		jmp p[202 * 4];
	}
}

// D3DXMatrixOrthoRH
extern "C" __declspec(naked) void __cdecl __E__203__() {
	__asm
	{
		jmp p[203 * 4];
	}
}

// D3DXMatrixPerspectiveFovLH
extern "C" __declspec(naked) void __cdecl __E__204__() {
	__asm
	{
		jmp p[204 * 4];
	}
}

// D3DXMatrixPerspectiveFovRH
extern "C" __declspec(naked) void __cdecl __E__205__() {
	__asm
	{
		jmp p[205 * 4];
	}
}

// D3DXMatrixPerspectiveLH
extern "C" __declspec(naked) void __cdecl __E__206__() {
	__asm
	{
		jmp p[206 * 4];
	}
}

// D3DXMatrixPerspectiveOffCenterLH
extern "C" __declspec(naked) void __cdecl __E__207__() {
	__asm
	{
		jmp p[207 * 4];
	}
}

// D3DXMatrixPerspectiveOffCenterRH
extern "C" __declspec(naked) void __cdecl __E__208__() {
	__asm
	{
		jmp p[208 * 4];
	}
}

// D3DXMatrixPerspectiveRH
extern "C" __declspec(naked) void __cdecl __E__209__() {
	__asm
	{
		jmp p[209 * 4];
	}
}

// D3DXMatrixReflect
extern "C" __declspec(naked) void __cdecl __E__210__() {
	__asm
	{
		jmp p[210 * 4];
	}
}

// D3DXMatrixRotationAxis
extern "C" __declspec(naked) void __cdecl __E__211__() {
	__asm
	{
		jmp p[211 * 4];
	}
}

// D3DXMatrixRotationQuaternion
extern "C" __declspec(naked) void __cdecl __E__212__() {
	__asm
	{
		jmp p[212 * 4];
	}
}

// D3DXMatrixRotationX
extern "C" __declspec(naked) void __cdecl __E__213__() {
	__asm
	{
		jmp p[213 * 4];
	}
}

// D3DXMatrixRotationY
extern "C" __declspec(naked) void __cdecl __E__214__() {
	__asm
	{
		jmp p[214 * 4];
	}
}

// D3DXMatrixRotationYawPitchRoll
extern "C" __declspec(naked) void __cdecl __E__215__() {
	__asm
	{
		jmp p[215 * 4];
	}
}

// D3DXMatrixRotationZ
extern "C" __declspec(naked) void __cdecl __E__216__() {
	__asm
	{
		jmp p[216 * 4];
	}
}

// D3DXMatrixScaling
extern "C" __declspec(naked) void __cdecl __E__217__() {
	__asm
	{
		jmp p[217 * 4];
	}
}

// D3DXMatrixShadow
extern "C" __declspec(naked) void __cdecl __E__218__() {
	__asm
	{
		jmp p[218 * 4];
	}
}

// D3DXMatrixTransformation
extern "C" __declspec(naked) void __cdecl __E__219__() {
	__asm
	{
		jmp p[219 * 4];
	}
}

// D3DXMatrixTransformation2D
extern "C" __declspec(naked) void __cdecl __E__220__() {
	__asm
	{
		jmp p[220 * 4];
	}
}

// D3DXMatrixTranslation
extern "C" __declspec(naked) void __cdecl __E__221__() {
	__asm
	{
		jmp p[221 * 4];
	}
}

// D3DXMatrixTranspose
extern "C" __declspec(naked) void __cdecl __E__222__() {
	__asm
	{
		jmp p[222 * 4];
	}
}

// D3DXOptimizeFaces
extern "C" __declspec(naked) void __cdecl __E__223__() {
	__asm
	{
		jmp p[223 * 4];
	}
}

// D3DXOptimizeVertices
extern "C" __declspec(naked) void __cdecl __E__224__() {
	__asm
	{
		jmp p[224 * 4];
	}
}

// D3DXPlaneFromPointNormal
extern "C" __declspec(naked) void __cdecl __E__225__() {
	__asm
	{
		jmp p[225 * 4];
	}
}

// D3DXPlaneFromPoints
extern "C" __declspec(naked) void __cdecl __E__226__() {
	__asm
	{
		jmp p[226 * 4];
	}
}

// D3DXPlaneIntersectLine
extern "C" __declspec(naked) void __cdecl __E__227__() {
	__asm
	{
		jmp p[227 * 4];
	}
}

// D3DXPlaneNormalize
extern "C" __declspec(naked) void __cdecl __E__228__() {
	__asm
	{
		jmp p[228 * 4];
	}
}

// D3DXPlaneTransform
extern "C" __declspec(naked) void __cdecl __E__229__() {
	__asm
	{
		jmp p[229 * 4];
	}
}

// D3DXPlaneTransformArray
extern "C" __declspec(naked) void __cdecl __E__230__() {
	__asm
	{
		jmp p[230 * 4];
	}
}

// D3DXPreprocessShader
extern "C" __declspec(naked) void __cdecl __E__231__() {
	__asm
	{
		jmp p[231 * 4];
	}
}

// D3DXPreprocessShaderFromFileA
extern "C" __declspec(naked) void __cdecl __E__232__() {
	__asm
	{
		jmp p[232 * 4];
	}
}

// D3DXPreprocessShaderFromFileW
extern "C" __declspec(naked) void __cdecl __E__233__() {
	__asm
	{
		jmp p[233 * 4];
	}
}

// D3DXPreprocessShaderFromResourceA
extern "C" __declspec(naked) void __cdecl __E__234__() {
	__asm
	{
		jmp p[234 * 4];
	}
}

// D3DXPreprocessShaderFromResourceW
extern "C" __declspec(naked) void __cdecl __E__235__() {
	__asm
	{
		jmp p[235 * 4];
	}
}

// D3DXQuaternionBaryCentric
extern "C" __declspec(naked) void __cdecl __E__236__() {
	__asm
	{
		jmp p[236 * 4];
	}
}

// D3DXQuaternionExp
extern "C" __declspec(naked) void __cdecl __E__237__() {
	__asm
	{
		jmp p[237 * 4];
	}
}

// D3DXQuaternionInverse
extern "C" __declspec(naked) void __cdecl __E__238__() {
	__asm
	{
		jmp p[238 * 4];
	}
}

// D3DXQuaternionLn
extern "C" __declspec(naked) void __cdecl __E__239__() {
	__asm
	{
		jmp p[239 * 4];
	}
}

// D3DXQuaternionMultiply
extern "C" __declspec(naked) void __cdecl __E__240__() {
	__asm
	{
		jmp p[240 * 4];
	}
}

// D3DXQuaternionNormalize
extern "C" __declspec(naked) void __cdecl __E__241__() {
	__asm
	{
		jmp p[241 * 4];
	}
}

// D3DXQuaternionRotationAxis
extern "C" __declspec(naked) void __cdecl __E__242__() {
	__asm
	{
		jmp p[242 * 4];
	}
}

// D3DXQuaternionRotationMatrix
extern "C" __declspec(naked) void __cdecl __E__243__() {
	__asm
	{
		jmp p[243 * 4];
	}
}

// D3DXQuaternionRotationYawPitchRoll
extern "C" __declspec(naked) void __cdecl __E__244__() {
	__asm
	{
		jmp p[244 * 4];
	}
}

// D3DXQuaternionSlerp
extern "C" __declspec(naked) void __cdecl __E__245__() {
	__asm
	{
		jmp p[245 * 4];
	}
}

// D3DXQuaternionSquad
extern "C" __declspec(naked) void __cdecl __E__246__() {
	__asm
	{
		jmp p[246 * 4];
	}
}

// D3DXQuaternionSquadSetup
extern "C" __declspec(naked) void __cdecl __E__247__() {
	__asm
	{
		jmp p[247 * 4];
	}
}

// D3DXQuaternionToAxisAngle
extern "C" __declspec(naked) void __cdecl __E__248__() {
	__asm
	{
		jmp p[248 * 4];
	}
}

// D3DXRectPatchSize
extern "C" __declspec(naked) void __cdecl __E__249__() {
	__asm
	{
		jmp p[249 * 4];
	}
}

// D3DXSHAdd
extern "C" __declspec(naked) void __cdecl __E__250__() {
	__asm
	{
		jmp p[250 * 4];
	}
}

// D3DXSHDot
extern "C" __declspec(naked) void __cdecl __E__251__() {
	__asm
	{
		jmp p[251 * 4];
	}
}

// D3DXSHEvalConeLight
extern "C" __declspec(naked) void __cdecl __E__252__() {
	__asm
	{
		jmp p[252 * 4];
	}
}

// D3DXSHEvalDirection
extern "C" __declspec(naked) void __cdecl __E__253__() {
	__asm
	{
		jmp p[253 * 4];
	}
}

// D3DXSHEvalDirectionalLight
extern "C" __declspec(naked) void __cdecl __E__254__() {
	__asm
	{
		jmp p[254 * 4];
	}
}

// D3DXSHEvalHemisphereLight
extern "C" __declspec(naked) void __cdecl __E__255__() {
	__asm
	{
		jmp p[255 * 4];
	}
}

// D3DXSHEvalSphericalLight
extern "C" __declspec(naked) void __cdecl __E__256__() {
	__asm
	{
		jmp p[256 * 4];
	}
}

// D3DXSHMultiply2
extern "C" __declspec(naked) void __cdecl __E__257__() {
	__asm
	{
		jmp p[257 * 4];
	}
}

// D3DXSHMultiply3
extern "C" __declspec(naked) void __cdecl __E__258__() {
	__asm
	{
		jmp p[258 * 4];
	}
}

// D3DXSHMultiply4
extern "C" __declspec(naked) void __cdecl __E__259__() {
	__asm
	{
		jmp p[259 * 4];
	}
}

// D3DXSHMultiply5
extern "C" __declspec(naked) void __cdecl __E__260__() {
	__asm
	{
		jmp p[260 * 4];
	}
}

// D3DXSHMultiply6
extern "C" __declspec(naked) void __cdecl __E__261__() {
	__asm
	{
		jmp p[261 * 4];
	}
}

// D3DXSHPRTCompSplitMeshSC
extern "C" __declspec(naked) void __cdecl __E__262__() {
	__asm
	{
		jmp p[262 * 4];
	}
}

// D3DXSHPRTCompSuperCluster
extern "C" __declspec(naked) void __cdecl __E__263__() {
	__asm
	{
		jmp p[263 * 4];
	}
}

// D3DXSHProjectCubeMap
extern "C" __declspec(naked) void __cdecl __E__264__() {
	__asm
	{
		jmp p[264 * 4];
	}
}

// D3DXSHRotate
extern "C" __declspec(naked) void __cdecl __E__265__() {
	__asm
	{
		jmp p[265 * 4];
	}
}

// D3DXSHRotateZ
extern "C" __declspec(naked) void __cdecl __E__266__() {
	__asm
	{
		jmp p[266 * 4];
	}
}

// D3DXSHScale
extern "C" __declspec(naked) void __cdecl __E__267__() {
	__asm
	{
		jmp p[267 * 4];
	}
}

// D3DXSaveMeshHierarchyToFileA
extern "C" __declspec(naked) void __cdecl __E__268__() {
	__asm
	{
		jmp p[268 * 4];
	}
}

// D3DXSaveMeshHierarchyToFileW
extern "C" __declspec(naked) void __cdecl __E__269__() {
	__asm
	{
		jmp p[269 * 4];
	}
}

// D3DXSaveMeshToXA
extern "C" __declspec(naked) void __cdecl __E__270__() {
	__asm
	{
		jmp p[270 * 4];
	}
}

// D3DXSaveMeshToXW
extern "C" __declspec(naked) void __cdecl __E__271__() {
	__asm
	{
		jmp p[271 * 4];
	}
}

// D3DXSavePRTBufferToFileA
extern "C" __declspec(naked) void __cdecl __E__272__() {
	__asm
	{
		jmp p[272 * 4];
	}
}

// D3DXSavePRTBufferToFileW
extern "C" __declspec(naked) void __cdecl __E__273__() {
	__asm
	{
		jmp p[273 * 4];
	}
}

// D3DXSavePRTCompBufferToFileA
extern "C" __declspec(naked) void __cdecl __E__274__() {
	__asm
	{
		jmp p[274 * 4];
	}
}

// D3DXSavePRTCompBufferToFileW
extern "C" __declspec(naked) void __cdecl __E__275__() {
	__asm
	{
		jmp p[275 * 4];
	}
}

// D3DXSaveSurfaceToFileA
extern "C" __declspec(naked) void __cdecl __E__276__() {
	__asm
	{
		jmp p[276 * 4];
	}
}

// D3DXSaveSurfaceToFileInMemory
extern "C" __declspec(naked) void __cdecl __E__277__() {
	__asm
	{
		jmp p[277 * 4];
	}
}

// D3DXSaveSurfaceToFileW
extern "C" __declspec(naked) void __cdecl __E__278__() {
	__asm
	{
		jmp p[278 * 4];
	}
}

// D3DXSaveTextureToFileA
extern "C" __declspec(naked) void __cdecl __E__279__() {
	__asm
	{
		jmp p[279 * 4];
	}
}

// D3DXSaveTextureToFileInMemory
extern "C" __declspec(naked) void __cdecl __E__280__() {
	__asm
	{
		jmp p[280 * 4];
	}
}

// D3DXSaveTextureToFileW
extern "C" __declspec(naked) void __cdecl __E__281__() {
	__asm
	{
		jmp p[281 * 4];
	}
}

// D3DXSaveVolumeToFileA
extern "C" __declspec(naked) void __cdecl __E__282__() {
	__asm
	{
		jmp p[282 * 4];
	}
}

// D3DXSaveVolumeToFileInMemory
extern "C" __declspec(naked) void __cdecl __E__283__() {
	__asm
	{
		jmp p[283 * 4];
	}
}

// D3DXSaveVolumeToFileW
extern "C" __declspec(naked) void __cdecl __E__284__() {
	__asm
	{
		jmp p[284 * 4];
	}
}

// D3DXSimplifyMesh
extern "C" __declspec(naked) void __cdecl __E__285__() {
	__asm
	{
		jmp p[285 * 4];
	}
}

// D3DXSphereBoundProbe
extern "C" __declspec(naked) void __cdecl __E__286__() {
	__asm
	{
		jmp p[286 * 4];
	}
}

// D3DXSplitMesh
extern "C" __declspec(naked) void __cdecl __E__287__() {
	__asm
	{
		jmp p[287 * 4];
	}
}

// D3DXTessellateNPatches
extern "C" __declspec(naked) void __cdecl __E__288__() {
	__asm
	{
		jmp p[288 * 4];
	}
}

// D3DXTessellateRectPatch
extern "C" __declspec(naked) void __cdecl __E__289__() {
	__asm
	{
		jmp p[289 * 4];
	}
}

// D3DXTessellateTriPatch
extern "C" __declspec(naked) void __cdecl __E__290__() {
	__asm
	{
		jmp p[290 * 4];
	}
}

// D3DXTriPatchSize
extern "C" __declspec(naked) void __cdecl __E__291__() {
	__asm
	{
		jmp p[291 * 4];
	}
}

// D3DXUVAtlasCreate
extern "C" __declspec(naked) void __cdecl __E__292__() {
	__asm
	{
		jmp p[292 * 4];
	}
}

// D3DXUVAtlasPack
extern "C" __declspec(naked) void __cdecl __E__293__() {
	__asm
	{
		jmp p[293 * 4];
	}
}

// D3DXUVAtlasPartition
extern "C" __declspec(naked) void __cdecl __E__294__() {
	__asm
	{
		jmp p[294 * 4];
	}
}

// D3DXValidMesh
extern "C" __declspec(naked) void __cdecl __E__295__() {
	__asm
	{
		jmp p[295 * 4];
	}
}

// D3DXValidPatchMesh
extern "C" __declspec(naked) void __cdecl __E__296__() {
	__asm
	{
		jmp p[296 * 4];
	}
}

// D3DXVec2BaryCentric
extern "C" __declspec(naked) void __cdecl __E__297__() {
	__asm
	{
		jmp p[297 * 4];
	}
}

// D3DXVec2CatmullRom
extern "C" __declspec(naked) void __cdecl __E__298__() {
	__asm
	{
		jmp p[298 * 4];
	}
}

// D3DXVec2Hermite
extern "C" __declspec(naked) void __cdecl __E__299__() {
	__asm
	{
		jmp p[299 * 4];
	}
}

// D3DXVec2Normalize
extern "C" __declspec(naked) void __cdecl __E__300__() {
	__asm
	{
		jmp p[300 * 4];
	}
}

// D3DXVec2Transform
extern "C" __declspec(naked) void __cdecl __E__301__() {
	__asm
	{
		jmp p[301 * 4];
	}
}

// D3DXVec2TransformArray
extern "C" __declspec(naked) void __cdecl __E__302__() {
	__asm
	{
		jmp p[302 * 4];
	}
}

// D3DXVec2TransformCoord
extern "C" __declspec(naked) void __cdecl __E__303__() {
	__asm
	{
		jmp p[303 * 4];
	}
}

// D3DXVec2TransformCoordArray
extern "C" __declspec(naked) void __cdecl __E__304__() {
	__asm
	{
		jmp p[304 * 4];
	}
}

// D3DXVec2TransformNormal
extern "C" __declspec(naked) void __cdecl __E__305__() {
	__asm
	{
		jmp p[305 * 4];
	}
}

// D3DXVec2TransformNormalArray
extern "C" __declspec(naked) void __cdecl __E__306__() {
	__asm
	{
		jmp p[306 * 4];
	}
}

// D3DXVec3BaryCentric
extern "C" __declspec(naked) void __cdecl __E__307__() {
	__asm
	{
		jmp p[307 * 4];
	}
}

// D3DXVec3CatmullRom
extern "C" __declspec(naked) void __cdecl __E__308__() {
	__asm
	{
		jmp p[308 * 4];
	}
}

// D3DXVec3Hermite
extern "C" __declspec(naked) void __cdecl __E__309__() {
	__asm
	{
		jmp p[309 * 4];
	}
}

// D3DXVec3Normalize
extern "C" __declspec(naked) void __cdecl __E__310__() {
	__asm
	{
		jmp p[310 * 4];
	}
}

// D3DXVec3Project
extern "C" __declspec(naked) void __cdecl __E__311__() {
	__asm
	{
		jmp p[311 * 4];
	}
}

// D3DXVec3ProjectArray
extern "C" __declspec(naked) void __cdecl __E__312__() {
	__asm
	{
		jmp p[312 * 4];
	}
}

// D3DXVec3Transform
extern "C" __declspec(naked) void __cdecl __E__313__() {
	__asm
	{
		jmp p[313 * 4];
	}
}

// D3DXVec3TransformArray
extern "C" __declspec(naked) void __cdecl __E__314__() {
	__asm
	{
		jmp p[314 * 4];
	}
}

// D3DXVec3TransformCoord
extern "C" __declspec(naked) void __cdecl __E__315__() {
	__asm
	{
		jmp p[315 * 4];
	}
}

// D3DXVec3TransformCoordArray
extern "C" __declspec(naked) void __cdecl __E__316__() {
	__asm
	{
		jmp p[316 * 4];
	}
}

// D3DXVec3TransformNormal
extern "C" __declspec(naked) void __cdecl __E__317__() {
	__asm
	{
		jmp p[317 * 4];
	}
}

// D3DXVec3TransformNormalArray
extern "C" __declspec(naked) void __cdecl __E__318__() {
	__asm
	{
		jmp p[318 * 4];
	}
}

// D3DXVec3Unproject
extern "C" __declspec(naked) void __cdecl __E__319__() {
	__asm
	{
		jmp p[319 * 4];
	}
}

// D3DXVec3UnprojectArray
extern "C" __declspec(naked) void __cdecl __E__320__() {
	__asm
	{
		jmp p[320 * 4];
	}
}

// D3DXVec4BaryCentric
extern "C" __declspec(naked) void __cdecl __E__321__() {
	__asm
	{
		jmp p[321 * 4];
	}
}

// D3DXVec4CatmullRom
extern "C" __declspec(naked) void __cdecl __E__322__() {
	__asm
	{
		jmp p[322 * 4];
	}
}

// D3DXVec4Cross
extern "C" __declspec(naked) void __cdecl __E__323__() {
	__asm
	{
		jmp p[323 * 4];
	}
}

// D3DXVec4Hermite
extern "C" __declspec(naked) void __cdecl __E__324__() {
	__asm
	{
		jmp p[324 * 4];
	}
}

// D3DXVec4Normalize
extern "C" __declspec(naked) void __cdecl __E__325__() {
	__asm
	{
		jmp p[325 * 4];
	}
}

// D3DXVec4Transform
extern "C" __declspec(naked) void __cdecl __E__326__() {
	__asm
	{
		jmp p[326 * 4];
	}
}

// D3DXVec4TransformArray
extern "C" __declspec(naked) void __cdecl __E__327__() {
	__asm
	{
		jmp p[327 * 4];
	}
}

// D3DXWeldVertices
extern "C" __declspec(naked) void __cdecl __E__328__() {
	__asm
	{
		jmp p[328 * 4];
	}
}

