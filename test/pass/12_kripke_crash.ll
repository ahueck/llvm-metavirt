; RUN: %apply-metavirt %s 2>&1 | %filecheck %s
; CHECK: Potential call targets:
; CHECK-NEXT: -----

; This is merely a crash reproducer for the function `derived_tys_for_base`. It outputs non-sensical stuff.
; The original implementation did not account for `typedef` derived types in the metadata dependency chain.

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define fastcc ptr @_ZNKSt17_Rb_tree_iteratorISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEEptEv() !dbg !8 {
  ret ptr null
}

define fastcc void @_ZN6Kripke4Core9DataStore14deleteVariableERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE() {
    #dbg_value(ptr null, !19, !DIExpression(), !56)
  %1 = call fastcc ptr @_ZNKSt17_Rb_tree_iteratorISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEEptEv()
  %2 = load ptr, ptr %1, align 8
  tail call void %2(ptr null)
  ret void
}

!llvm.dbg.cu = !{!0, !3, !5}
!llvm.module.flags = !{!7}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang version 21.1.4", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !2, globals: !2, imports: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "Kripke/src/kripke.cpp", directory: "Kripke/build", checksumkind: CSK_MD5, checksum: "cee5966918521ec2d08710799a4069d0")
!2 = !{}
!3 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !4, producer: "clang version 21.1.4", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !2, globals: !2, imports: !2, splitDebugInlining: false, nameTableKind: None)
!4 = !DIFile(filename: "Kripke/src/Kripke/Core/DataStore.cpp", directory: "Kripke/build", checksumkind: CSK_MD5, checksum: "e84f2562c4dfaef03af652418efe41df")
!5 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !6, producer: "clang version 21.1.4", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !2, globals: !2, imports: !2, splitDebugInlining: false, nameTableKind: None)
!6 = !DIFile(filename: "Kripke/src/Kripke/Generate/Data.cpp", directory: "Kripke/build", checksumkind: CSK_MD5, checksum: "348479b2b52d3d6825d6b1ac5d5da6e9")
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = distinct !DISubprogram(name: "operator->", linkageName: "_ZNKSt17_Rb_tree_iteratorISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEEptEv", scope: !10, file: !9, line: 396, type: !12, scopeLine: 397, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, declaration: !18, retainedNodes: !2)
!9 = !DIFile(filename: "include/c++/15.2.0/bits/stl_tree.h", directory: "/work", checksumkind: CSK_MD5, checksum: "c0874190fac1e81e66ba299a8440f4e4")
!10 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_Rb_tree_iterator<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> >", scope: !11, file: !9, line: 372, size: 64, flags: DIFlagTypePassByValue | DIFlagNonTrivial, elements: !2, templateParams: !2, identifier: "_ZTSSt17_Rb_tree_iteratorISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEE")
!11 = !DINamespace(name: "std", scope: null)
!12 = !DISubroutineType(types: !13)
!13 = !{!14}
!14 = !DIDerivedType(tag: DW_TAG_typedef, name: "pointer", scope: !10, file: !9, line: 376, baseType: !15)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!16 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *>", scope: !11, file: !17, line: 302, size: 320, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !2, templateParams: !2, identifier: "_ZTSSt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEE")
!17 = !DIFile(filename: "include/c++/15.2.0/bits/stl_pair.h", directory: "/work", checksumkind: CSK_MD5, checksum: "e767ec7b42d6a51baeeb74d932a1174f")
!18 = !DISubprogram(name: "operator->", linkageName: "_ZNKSt17_Rb_tree_iteratorISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEEptEv", scope: !10, file: !9, line: 396, type: !12, scopeLine: 396, flags: DIFlagPrototyped, spFlags: DISPFlagOptimized)
!19 = !DILocalVariable(name: "it", scope: !20, file: !21, line: 40, type: !29)
!20 = distinct !DISubprogram(name: "deleteVariable", linkageName: "_ZN6Kripke4Core9DataStore14deleteVariableERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE", scope: !22, file: !21, line: 39, type: !26, scopeLine: 39, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !3, declaration: !28, retainedNodes: !2)
!21 = !DIFile(filename: "src/Kripke/Core/DataStore.cpp", directory: "Kripke", checksumkind: CSK_MD5, checksum: "e84f2562c4dfaef03af652418efe41df")
!22 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "DataStore", scope: !24, file: !23, line: 23, size: 384, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !2, identifier: "_ZTSN6Kripke4Core9DataStoreE")
!23 = !DIFile(filename: "src/Kripke/Core/DataStore.h", directory: "Kripke", checksumkind: CSK_MD5, checksum: "ab3167a1c08197b20d55ccb9154d43f3")
!24 = !DINamespace(name: "Core", scope: !25)
!25 = !DINamespace(name: "Kripke", scope: null)
!26 = distinct !DISubroutineType(types: !27)
!27 = !{null}
!28 = !DISubprogram(name: "deleteVariable", linkageName: "_ZN6Kripke4Core9DataStore14deleteVariableERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE", scope: !22, file: !23, line: 40, type: !26, scopeLine: 40, flags: DIFlagPublic | DIFlagPrototyped, spFlags: DISPFlagOptimized)
!29 = !DIDerivedType(tag: DW_TAG_typedef, name: "iterator", scope: !31, file: !30, line: 179, baseType: !32, flags: DIFlagPublic)
!30 = !DIFile(filename: "include/c++/15.2.0/bits/stl_map.h", directory: "/work", checksumkind: CSK_MD5, checksum: "97a36b70cb7fa35e0e6b43ae0d6dc125")
!31 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> > >", scope: !11, file: !30, line: 105, size: 384, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !2, templateParams: !2, identifier: "_ZTSSt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarESt4lessIS5_ESaISt4pairIKS5_S9_EEE")
!32 = !DIDerivedType(tag: DW_TAG_typedef, name: "iterator", scope: !33, file: !9, line: 1448, baseType: !53, flags: DIFlagPublic)
!33 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "_Rb_tree<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *>, std::_Select1st<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> >, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> > >", scope: !11, file: !9, line: 1020, size: 384, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !34, templateParams: !2, identifier: "_ZTSSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_PN6Kripke4Core7BaseVarEESt10_Select1stISC_ESt4lessIS5_ESaISC_EE")
!34 = !{!35}
!35 = !DIDerivedType(tag: DW_TAG_member, name: "_M_impl", scope: !33, file: !9, line: 1349, baseType: !36, size: 384, flags: DIFlagProtected)
!36 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_Rb_tree_impl<std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, true>", scope: !33, file: !9, line: 1303, size: 384, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !37, templateParams: !2, identifier: "_ZTSNSt8_Rb_treeINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4pairIKS5_PN6Kripke4Core7BaseVarEESt10_Select1stISC_ESt4lessIS5_ESaISC_EE13_Rb_tree_implISG_Lb1EEE")
!37 = !{!38}
!38 = !DIDerivedType(tag: DW_TAG_inheritance, scope: !36, baseType: !39, extraData: i32 0)
!39 = !DIDerivedType(tag: DW_TAG_typedef, name: "_Node_allocator", scope: !33, file: !9, line: 1033, baseType: !40)
!40 = !DIDerivedType(tag: DW_TAG_typedef, name: "other", scope: !42, file: !41, line: 128, baseType: !48)
!41 = !DIFile(filename: "include/c++/15.2.0/ext/alloc_traits.h", directory: "/work", checksumkind: CSK_MD5, checksum: "b538cf34d296f3161c57558eea3bca8b")
!42 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "rebind<std::_Rb_tree_node<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> > >", scope: !43, file: !41, line: 127, size: 8, flags: DIFlagTypePassByValue, elements: !2, templateParams: !45, identifier: "_ZTSN9__gnu_cxx14__alloc_traitsISaISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEESD_E6rebindISt13_Rb_tree_nodeISD_EEE")
!43 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "__alloc_traits<std::allocator<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> >, std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> >", scope: !44, file: !41, line: 47, size: 8, flags: DIFlagTypePassByValue, elements: !2, templateParams: !2, identifier: "_ZTSN9__gnu_cxx14__alloc_traitsISaISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEESD_EE")
!44 = !DINamespace(name: "__gnu_cxx", scope: null)
!45 = !{!46}
!46 = !DITemplateTypeParameter(name: "_Tp", type: !47)
!47 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_Rb_tree_node<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> >", scope: !11, file: !9, line: 214, size: 576, flags: DIFlagTypePassByValue, elements: !2, templateParams: !2, identifier: "_ZTSSt13_Rb_tree_nodeISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEE")
!48 = !DIDerivedType(tag: DW_TAG_typedef, name: "rebind_alloc<std::_Rb_tree_node<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> > >", scope: !50, file: !49, line: 599, baseType: !51)
!49 = !DIFile(filename: "include/c++/15.2.0/bits/alloc_traits.h", directory: "/work", checksumkind: CSK_MD5, checksum: "c2f5862787920c2fc07b4f499e362a00")
!50 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "allocator_traits<std::allocator<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> > >", scope: !11, file: !49, line: 560, size: 8, flags: DIFlagTypePassByValue, elements: !2, templateParams: !2, identifier: "_ZTSSt16allocator_traitsISaISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEEE")
!51 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "allocator<std::_Rb_tree_node<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> > >", scope: !11, file: !52, line: 133, size: 8, flags: DIFlagTypePassByReference | DIFlagNonTrivial, elements: !2, templateParams: !2, identifier: "_ZTSSaISt13_Rb_tree_nodeISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEEE")
!52 = !DIFile(filename: "include/c++/15.2.0/bits/allocator.h", directory: "/work", checksumkind: CSK_MD5, checksum: "d18c16b3c9c3ab2e7d487de90ec27d17")
!53 = !DIDerivedType(tag: DW_TAG_typedef, name: "_Iterator", scope: !54, file: !9, line: 684, baseType: !10)
!54 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_Node_traits<std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *>, std::pair<const std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, Kripke::Core::BaseVar *> *>", scope: !55, file: !9, line: 677, size: 8, flags: DIFlagTypePassByValue, elements: !2, templateParams: !2, identifier: "_ZTSNSt9__rb_tree12_Node_traitsISt4pairIKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPN6Kripke4Core7BaseVarEEPSD_EE")
!55 = !DINamespace(name: "__rb_tree", scope: !11)
!56 = !DILocation(line: 0, scope: !20)
