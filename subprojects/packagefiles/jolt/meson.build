project('jolt', 'cpp',
	license : 'MIT',
	license_files : [ 'LICENSE' ],
	version : '5.0.0',
	meson_version : '>=1.3.0',
    default_options : [
        'cpp_std=c++17'
    ]
)

root = meson.current_source_dir() / 'Jolt'

# Source files
all = [
	root/'AABBTree/AABBTreeBuilder.cpp',
	root/'AABBTree/AABBTreeBuilder.h',
	root/'AABBTree/AABBTreeToBuffer.h',
	root/'AABBTree/NodeCodec/NodeCodecQuadTreeHalfFloat.h',
	root/'AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h',
	root/'Core/ARMNeon.h',
	root/'Core/Atomics.h',
	root/'Core/ByteBuffer.h',
	root/'Core/Color.cpp',
	root/'Core/Color.h',
	root/'Core/Core.h',
	root/'Core/Factory.cpp',
	root/'Core/Factory.h',
	root/'Core/FixedSizeFreeList.h',
	root/'Core/FixedSizeFreeList.inl',
	root/'Core/FPControlWord.h',
	root/'Core/FPException.h',
	root/'Core/FPFlushDenormals.h',
	root/'Core/HashCombine.h',
	root/'Core/InsertionSort.h',
	root/'Core/IssueReporting.cpp',
	root/'Core/IssueReporting.h',
	root/'Core/JobSystem.h',
	root/'Core/JobSystem.inl',
	root/'Core/JobSystemSingleThreaded.cpp',
	root/'Core/JobSystemSingleThreaded.h',
	root/'Core/JobSystemThreadPool.cpp',
	root/'Core/JobSystemThreadPool.h',
	root/'Core/JobSystemWithBarrier.cpp',
	root/'Core/JobSystemWithBarrier.h',
	root/'Core/LinearCurve.cpp',
	root/'Core/LinearCurve.h',
	root/'Core/LockFreeHashMap.h',
	root/'Core/LockFreeHashMap.inl',
	root/'Core/Memory.cpp',
	root/'Core/Memory.h',
	root/'Core/Mutex.h',
	root/'Core/MutexArray.h',
	root/'Core/NonCopyable.h',
	root/'Core/Profiler.cpp',
	root/'Core/Profiler.h',
	root/'Core/Profiler.inl',
	root/'Core/QuickSort.h',
	root/'Core/Reference.h',
	root/'Core/Result.h',
	root/'Core/RTTI.cpp',
	root/'Core/RTTI.h',
	root/'Core/Semaphore.cpp',
	root/'Core/Semaphore.h',
	root/'Core/StaticArray.h',
	root/'Core/StreamIn.h',
	root/'Core/StreamOut.h',
	root/'Core/StreamUtils.h',
	root/'Core/StreamWrapper.h',
	root/'Core/StringTools.cpp',
	root/'Core/StringTools.h',
	root/'Core/STLAlignedAllocator.h',
	root/'Core/STLAllocator.h',
	root/'Core/STLTempAllocator.h',
	root/'Core/TempAllocator.h',
	root/'Core/TickCounter.cpp',
	root/'Core/TickCounter.h',
	root/'Core/UnorderedMap.h',
	root/'Core/UnorderedSet.h',
	root/'Geometry/AABox.h',
	root/'Geometry/AABox4.h',
	root/'Geometry/ClipPoly.h',
	root/'Geometry/ClosestPoint.h',
	root/'Geometry/ConvexHullBuilder.cpp',
	root/'Geometry/ConvexHullBuilder.h',
	root/'Geometry/ConvexHullBuilder2D.cpp',
	root/'Geometry/ConvexHullBuilder2D.h',
	root/'Geometry/ConvexSupport.h',
	root/'Geometry/Ellipse.h',
	root/'Geometry/EPAConvexHullBuilder.h',
	root/'Geometry/EPAPenetrationDepth.h',
	root/'Geometry/GJKClosestPoint.h',
	root/'Geometry/IndexedTriangle.h',
	root/'Geometry/Indexify.cpp',
	root/'Geometry/Indexify.h',
	root/'Geometry/MortonCode.h',
	root/'Geometry/OrientedBox.cpp',
	root/'Geometry/OrientedBox.h',
	root/'Geometry/Plane.h',
	root/'Geometry/RayAABox.h',
	root/'Geometry/RayAABox8.h',
	root/'Geometry/RayCapsule.h',
	root/'Geometry/RayCylinder.h',
	root/'Geometry/RaySphere.h',
	root/'Geometry/RayTriangle.h',
	root/'Geometry/RayTriangle8.h',
	root/'Geometry/Sphere.h',
	root/'Geometry/Triangle.h',
	root/'Jolt.cmake',
	root/'Jolt.h',
	root/'Math/DMat44.h',
	root/'Math/DMat44.inl',
	root/'Math/Double3.h',
	root/'Math/DVec3.h',
	root/'Math/DVec3.inl',
	root/'Math/DynMatrix.h',
	root/'Math/EigenValueSymmetric.h',
	root/'Math/FindRoot.h',
	root/'Math/Float2.h',
	root/'Math/Float3.h',
	root/'Math/Float4.h',
	root/'Math/GaussianElimination.h',
	root/'Math/HalfFloat.h',
	root/'Math/Mat44.h',
	root/'Math/Mat44.inl',
	root/'Math/Math.h',
	root/'Math/MathTypes.h',
	root/'Math/Matrix.h',
	root/'Math/Quat.h',
	root/'Math/Quat.inl',
	root/'Math/Real.h',
	root/'Math/Swizzle.h',
	root/'Math/Trigonometry.h',
	root/'Math/UVec4.h',
	root/'Math/UVec4.inl',
	root/'Math/UVec8.h',
	root/'Math/UVec8.inl',
	root/'Math/Vec3.cpp',
	root/'Math/Vec3.h',
	root/'Math/Vec3.inl',
	root/'Math/Vec4.h',
	root/'Math/Vec4.inl',
	root/'Math/Vec8.h',
	root/'Math/Vec8.inl',
	root/'Math/Vector.h',
	root/'ObjectStream/GetPrimitiveTypeOfType.h',
	root/'ObjectStream/ObjectStream.cpp',
	root/'ObjectStream/ObjectStream.h',
	root/'ObjectStream/ObjectStreamBinaryIn.cpp',
	root/'ObjectStream/ObjectStreamBinaryIn.h',
	root/'ObjectStream/ObjectStreamBinaryOut.cpp',
	root/'ObjectStream/ObjectStreamBinaryOut.h',
	root/'ObjectStream/ObjectStreamIn.cpp',
	root/'ObjectStream/ObjectStreamIn.h',
	root/'ObjectStream/ObjectStreamOut.cpp',
	root/'ObjectStream/ObjectStreamOut.h',
	root/'ObjectStream/ObjectStreamTextIn.cpp',
	root/'ObjectStream/ObjectStreamTextIn.h',
	root/'ObjectStream/ObjectStreamTextOut.cpp',
	root/'ObjectStream/ObjectStreamTextOut.h',
	root/'ObjectStream/ObjectStreamTypes.h',
	root/'ObjectStream/SerializableAttribute.h',
	root/'ObjectStream/SerializableAttributeEnum.h',
	root/'ObjectStream/SerializableAttributeTyped.h',
	root/'ObjectStream/SerializableObject.cpp',
	root/'ObjectStream/SerializableObject.h',
	root/'ObjectStream/TypeDeclarations.cpp',
	root/'ObjectStream/TypeDeclarations.h',
	root/'Physics/Body/AllowedDOFs.h',
	root/'Physics/Body/Body.cpp',
	root/'Physics/Body/Body.h',
	root/'Physics/Body/Body.inl',
	root/'Physics/Body/BodyAccess.cpp',
	root/'Physics/Body/BodyAccess.h',
	root/'Physics/Body/BodyActivationListener.h',
	root/'Physics/Body/BodyCreationSettings.cpp',
	root/'Physics/Body/BodyCreationSettings.h',
	root/'Physics/Body/BodyFilter.h',
	root/'Physics/Body/BodyID.h',
	root/'Physics/Body/BodyInterface.cpp',
	root/'Physics/Body/BodyInterface.h',
	root/'Physics/Body/BodyLock.h',
	root/'Physics/Body/BodyLockInterface.h',
	root/'Physics/Body/BodyLockMulti.h',
	root/'Physics/Body/BodyManager.cpp',
	root/'Physics/Body/BodyManager.h',
	root/'Physics/Body/BodyPair.h',
	root/'Physics/Body/BodyType.h',
	root/'Physics/Body/MassProperties.cpp',
	root/'Physics/Body/MassProperties.h',
	root/'Physics/Body/MotionProperties.cpp',
	root/'Physics/Body/MotionProperties.h',
	root/'Physics/Body/MotionProperties.inl',
	root/'Physics/Body/MotionQuality.h',
	root/'Physics/Body/MotionType.h',
	root/'Physics/Character/Character.cpp',
	root/'Physics/Character/Character.h',
	root/'Physics/Character/CharacterBase.cpp',
	root/'Physics/Character/CharacterBase.h',
	root/'Physics/Character/CharacterVirtual.cpp',
	root/'Physics/Character/CharacterVirtual.h',
	root/'Physics/Collision/AABoxCast.h',
	root/'Physics/Collision/ActiveEdgeMode.h',
	root/'Physics/Collision/ActiveEdges.h',
	root/'Physics/Collision/BackFaceMode.h',
	root/'Physics/Collision/BroadPhase/BroadPhase.cpp',
	root/'Physics/Collision/BroadPhase/BroadPhase.h',
	root/'Physics/Collision/BroadPhase/BroadPhaseBruteForce.cpp',
	root/'Physics/Collision/BroadPhase/BroadPhaseBruteForce.h',
	root/'Physics/Collision/BroadPhase/BroadPhaseLayer.h',
	root/'Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceMask.h',
	root/'Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h',
	root/'Physics/Collision/BroadPhase/BroadPhaseQuadTree.cpp',
	root/'Physics/Collision/BroadPhase/BroadPhaseQuadTree.h',
	root/'Physics/Collision/BroadPhase/BroadPhaseQuery.h',
	root/'Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterMask.h',
	root/'Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h',
	root/'Physics/Collision/BroadPhase/QuadTree.cpp',
	root/'Physics/Collision/BroadPhase/QuadTree.h',
	root/'Physics/Collision/CastConvexVsTriangles.cpp',
	root/'Physics/Collision/CastConvexVsTriangles.h',
	root/'Physics/Collision/CastSphereVsTriangles.cpp',
	root/'Physics/Collision/CastSphereVsTriangles.h',
	root/'Physics/Collision/CastResult.h',
	root/'Physics/Collision/CollectFacesMode.h',
	root/'Physics/Collision/CollideConvexVsTriangles.cpp',
	root/'Physics/Collision/CollideConvexVsTriangles.h',
	root/'Physics/Collision/CollidePointResult.h',
	root/'Physics/Collision/CollideShape.h',
	root/'Physics/Collision/CollideSoftBodyVerticesVsTriangles.h',
	root/'Physics/Collision/CollideSphereVsTriangles.cpp',
	root/'Physics/Collision/CollideSphereVsTriangles.h',
	root/'Physics/Collision/CollisionCollector.h',
	root/'Physics/Collision/CollisionCollectorImpl.h',
	root/'Physics/Collision/CollisionDispatch.cpp',
	root/'Physics/Collision/CollisionDispatch.h',
	root/'Physics/Collision/CollisionGroup.cpp',
	root/'Physics/Collision/CollisionGroup.h',
	root/'Physics/Collision/ContactListener.h',
	root/'Physics/Collision/EstimateCollisionResponse.cpp',
	root/'Physics/Collision/EstimateCollisionResponse.h',
	root/'Physics/Collision/GroupFilter.cpp',
	root/'Physics/Collision/GroupFilter.h',
	root/'Physics/Collision/GroupFilterTable.cpp',
	root/'Physics/Collision/GroupFilterTable.h',
	root/'Physics/Collision/InternalEdgeRemovingCollector.h',
	root/'Physics/Collision/ManifoldBetweenTwoFaces.cpp',
	root/'Physics/Collision/ManifoldBetweenTwoFaces.h',
	root/'Physics/Collision/NarrowPhaseQuery.cpp',
	root/'Physics/Collision/NarrowPhaseQuery.h',
	root/'Physics/Collision/NarrowPhaseStats.cpp',
	root/'Physics/Collision/NarrowPhaseStats.h',
	root/'Physics/Collision/ObjectLayer.h',
	root/'Physics/Collision/ObjectLayerPairFilterMask.h',
	root/'Physics/Collision/ObjectLayerPairFilterTable.h',
	root/'Physics/Collision/PhysicsMaterial.cpp',
	root/'Physics/Collision/PhysicsMaterial.h',
	root/'Physics/Collision/PhysicsMaterialSimple.cpp',
	root/'Physics/Collision/PhysicsMaterialSimple.h',
	root/'Physics/Collision/RayCast.h',
	root/'Physics/Collision/Shape/BoxShape.cpp',
	root/'Physics/Collision/Shape/BoxShape.h',
	root/'Physics/Collision/Shape/CapsuleShape.cpp',
	root/'Physics/Collision/Shape/CapsuleShape.h',
	root/'Physics/Collision/Shape/CompoundShape.cpp',
	root/'Physics/Collision/Shape/CompoundShape.h',
	root/'Physics/Collision/Shape/CompoundShapeVisitors.h',
	root/'Physics/Collision/Shape/ConvexHullShape.cpp',
	root/'Physics/Collision/Shape/ConvexHullShape.h',
	root/'Physics/Collision/Shape/ConvexShape.cpp',
	root/'Physics/Collision/Shape/ConvexShape.h',
	root/'Physics/Collision/Shape/CylinderShape.cpp',
	root/'Physics/Collision/Shape/CylinderShape.h',
	root/'Physics/Collision/Shape/DecoratedShape.cpp',
	root/'Physics/Collision/Shape/DecoratedShape.h',
	root/'Physics/Collision/Shape/GetTrianglesContext.h',
	root/'Physics/Collision/Shape/HeightFieldShape.cpp',
	root/'Physics/Collision/Shape/HeightFieldShape.h',
	root/'Physics/Collision/Shape/MeshShape.cpp',
	root/'Physics/Collision/Shape/MeshShape.h',
	root/'Physics/Collision/Shape/MutableCompoundShape.cpp',
	root/'Physics/Collision/Shape/MutableCompoundShape.h',
	root/'Physics/Collision/Shape/OffsetCenterOfMassShape.cpp',
	root/'Physics/Collision/Shape/OffsetCenterOfMassShape.h',
	root/'Physics/Collision/Shape/PolyhedronSubmergedVolumeCalculator.h',
	root/'Physics/Collision/Shape/RotatedTranslatedShape.cpp',
	root/'Physics/Collision/Shape/RotatedTranslatedShape.h',
	root/'Physics/Collision/Shape/ScaledShape.cpp',
	root/'Physics/Collision/Shape/ScaledShape.h',
	root/'Physics/Collision/Shape/ScaleHelpers.h',
	root/'Physics/Collision/Shape/Shape.cpp',
	root/'Physics/Collision/Shape/Shape.h',
	root/'Physics/Collision/Shape/SphereShape.cpp',
	root/'Physics/Collision/Shape/SphereShape.h',
	root/'Physics/Collision/Shape/StaticCompoundShape.cpp',
	root/'Physics/Collision/Shape/StaticCompoundShape.h',
	root/'Physics/Collision/Shape/SubShapeID.h',
	root/'Physics/Collision/Shape/SubShapeIDPair.h',
	root/'Physics/Collision/Shape/TaperedCapsuleShape.cpp',
	root/'Physics/Collision/Shape/TaperedCapsuleShape.gliffy',
	root/'Physics/Collision/Shape/TaperedCapsuleShape.h',
	root/'Physics/Collision/Shape/TriangleShape.cpp',
	root/'Physics/Collision/Shape/TriangleShape.h',
	root/'Physics/Collision/ShapeCast.h',
	root/'Physics/Collision/ShapeFilter.h',
	root/'Physics/Collision/SortReverseAndStore.h',
	root/'Physics/Collision/TransformedShape.cpp',
	root/'Physics/Collision/TransformedShape.h',
	root/'Physics/Constraints/CalculateSolverSteps.h',
	root/'Physics/Constraints/ConeConstraint.cpp',
	root/'Physics/Constraints/ConeConstraint.h',
	root/'Physics/Constraints/Constraint.cpp',
	root/'Physics/Constraints/Constraint.h',
	root/'Physics/Constraints/ConstraintManager.cpp',
	root/'Physics/Constraints/ConstraintManager.h',
	root/'Physics/Constraints/ConstraintPart/AngleConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/AxisConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/DualAxisConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/GearConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/HingeRotationConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/IndependentAxisConstraintPart.',
	root/'Physics/Constraints/ConstraintPart/PointConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/RackAndPinionConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/RotationEulerConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/RotationQuatConstraintPart.h',
	root/'Physics/Constraints/ConstraintPart/SpringPart.h',
	root/'Physics/Constraints/ConstraintPart/SwingTwistConstraintPart.h',
	root/'Physics/Constraints/ContactConstraintManager.cpp',
	root/'Physics/Constraints/ContactConstraintManager.h',
	root/'Physics/Constraints/DistanceConstraint.cpp',
	root/'Physics/Constraints/DistanceConstraint.h',
	root/'Physics/Constraints/FixedConstraint.cpp',
	root/'Physics/Constraints/FixedConstraint.h',
	root/'Physics/Constraints/GearConstraint.cpp',
	root/'Physics/Constraints/GearConstraint.h',
	root/'Physics/Constraints/HingeConstraint.cpp',
	root/'Physics/Constraints/HingeConstraint.h',
	root/'Physics/Constraints/MotorSettings.cpp',
	root/'Physics/Constraints/MotorSettings.h',
	root/'Physics/Constraints/PathConstraint.cpp',
	root/'Physics/Constraints/PathConstraint.h',
	root/'Physics/Constraints/PathConstraintPath.cpp',
	root/'Physics/Constraints/PathConstraintPath.h',
	root/'Physics/Constraints/PathConstraintPathHermite.cpp',
	root/'Physics/Constraints/PathConstraintPathHermite.h',
	root/'Physics/Constraints/PointConstraint.cpp',
	root/'Physics/Constraints/PointConstraint.h',
	root/'Physics/Constraints/PulleyConstraint.cpp',
	root/'Physics/Constraints/PulleyConstraint.h',
	root/'Physics/Constraints/RackAndPinionConstraint.cpp',
	root/'Physics/Constraints/RackAndPinionConstraint.h',
	root/'Physics/Constraints/SixDOFConstraint.cpp',
	root/'Physics/Constraints/SixDOFConstraint.h',
	root/'Physics/Constraints/SliderConstraint.cpp',
	root/'Physics/Constraints/SliderConstraint.h',
	root/'Physics/Constraints/SpringSettings.cpp',
	root/'Physics/Constraints/SpringSettings.h',
	root/'Physics/Constraints/SwingTwistConstraint.cpp',
	root/'Physics/Constraints/SwingTwistConstraint.h',
	root/'Physics/Constraints/TwoBodyConstraint.cpp',
	root/'Physics/Constraints/TwoBodyConstraint.h',
	root/'Physics/DeterminismLog.cpp',
	root/'Physics/DeterminismLog.h',
	root/'Physics/EActivation.h',
	root/'Physics/EPhysicsUpdateError.h',
	root/'Physics/IslandBuilder.cpp',
	root/'Physics/IslandBuilder.h',
	root/'Physics/LargeIslandSplitter.cpp',
	root/'Physics/LargeIslandSplitter.h',
	root/'Physics/PhysicsLock.cpp',
	root/'Physics/PhysicsLock.h',
	root/'Physics/PhysicsScene.cpp',
	root/'Physics/PhysicsScene.h',
	root/'Physics/PhysicsSettings.h',
	root/'Physics/PhysicsStepListener.h',
	root/'Physics/PhysicsSystem.cpp',
	root/'Physics/PhysicsSystem.h',
	root/'Physics/PhysicsUpdateContext.cpp',
	root/'Physics/PhysicsUpdateContext.h',
	root/'Physics/Ragdoll/Ragdoll.cpp',
	root/'Physics/Ragdoll/Ragdoll.h',
	root/'Physics/SoftBody/SoftBodyContactListener.h',
	root/'Physics/SoftBody/SoftBodyCreationSettings.cpp',
	root/'Physics/SoftBody/SoftBodyCreationSettings.h',
	root/'Physics/SoftBody/SoftBodyManifold.h',
	root/'Physics/SoftBody/SoftBodyMotionProperties.h',
	root/'Physics/SoftBody/SoftBodyMotionProperties.cpp',
	root/'Physics/SoftBody/SoftBodyShape.cpp',
	root/'Physics/SoftBody/SoftBodyShape.h',
	root/'Physics/SoftBody/SoftBodySharedSettings.cpp',
	root/'Physics/SoftBody/SoftBodySharedSettings.h',
	root/'Physics/SoftBody/SoftBodyVertex.h',
	root/'Physics/StateRecorder.h',
	root/'Physics/StateRecorderImpl.cpp',
	root/'Physics/StateRecorderImpl.h',
	root/'Physics/Vehicle/MotorcycleController.cpp',
	root/'Physics/Vehicle/MotorcycleController.h',
	root/'Physics/Vehicle/TrackedVehicleController.cpp',
	root/'Physics/Vehicle/TrackedVehicleController.h',
	root/'Physics/Vehicle/VehicleAntiRollBar.cpp',
	root/'Physics/Vehicle/VehicleAntiRollBar.h',
	root/'Physics/Vehicle/VehicleCollisionTester.cpp',
	root/'Physics/Vehicle/VehicleCollisionTester.h',
	root/'Physics/Vehicle/VehicleConstraint.cpp',
	root/'Physics/Vehicle/VehicleConstraint.h',
	root/'Physics/Vehicle/VehicleController.cpp',
	root/'Physics/Vehicle/VehicleController.h',
	root/'Physics/Vehicle/VehicleDifferential.cpp',
	root/'Physics/Vehicle/VehicleDifferential.h',
	root/'Physics/Vehicle/VehicleEngine.cpp',
	root/'Physics/Vehicle/VehicleEngine.h',
	root/'Physics/Vehicle/VehicleTrack.cpp',
	root/'Physics/Vehicle/VehicleTrack.h',
	root/'Physics/Vehicle/VehicleTransmission.cpp',
	root/'Physics/Vehicle/VehicleTransmission.h',
	root/'Physics/Vehicle/Wheel.cpp',
	root/'Physics/Vehicle/Wheel.h',
	root/'Physics/Vehicle/WheeledVehicleController.cpp',
	root/'Physics/Vehicle/WheeledVehicleController.h',
	root/'RegisterTypes.cpp',
	root/'RegisterTypes.h',
	root/'Renderer/DebugRenderer.cpp',
	root/'Renderer/DebugRenderer.h',
	root/'Renderer/DebugRendererPlayback.cpp',
	root/'Renderer/DebugRendererPlayback.h',
	root/'Renderer/DebugRendererRecorder.cpp',
	root/'Renderer/DebugRendererRecorder.h',
	root/'Renderer/DebugRendererSimple.cpp',
	root/'Renderer/DebugRendererSimple.h',
	root/'Skeleton/SkeletalAnimation.cpp',
	root/'Skeleton/SkeletalAnimation.h',
	root/'Skeleton/Skeleton.cpp',
	root/'Skeleton/Skeleton.h',
	root/'Skeleton/SkeletonMapper.cpp',
	root/'Skeleton/SkeletonMapper.h',
	root/'Skeleton/SkeletonPose.cpp',
	root/'Skeleton/SkeletonPose.h',
	root/'TriangleGrouper/TriangleGrouper.h',
	root/'TriangleGrouper/TriangleGrouperClosestCentroid.cpp',
	root/'TriangleGrouper/TriangleGrouperClosestCentroid.h',
	root/'TriangleGrouper/TriangleGrouperMorton.cpp',
	root/'TriangleGrouper/TriangleGrouperMorton.h',
	root/'TriangleSplitter/TriangleSplitter.cpp',
	root/'TriangleSplitter/TriangleSplitter.h',
	root/'TriangleSplitter/TriangleSplitterBinning.cpp',
	root/'TriangleSplitter/TriangleSplitterBinning.h',
	root/'TriangleSplitter/TriangleSplitterFixedLeafSize.cpp',
	root/'TriangleSplitter/TriangleSplitterFixedLeafSize.h',
	root/'TriangleSplitter/TriangleSplitterLongestAxis.cpp',
	root/'TriangleSplitter/TriangleSplitterLongestAxis.h',
	root/'TriangleSplitter/TriangleSplitterMean.cpp',
	root/'TriangleSplitter/TriangleSplitterMean.h',
	root/'TriangleSplitter/TriangleSplitterMorton.cpp',
	root/'TriangleSplitter/TriangleSplitterMorton.h',
]

headers = []
sources = []

foreach item : all
    if item.endswith('.h') or item.endswith('.inl')
        headers += item
    elif item.endswith('.cpp')
        sources += item
    endif
endforeach

args = []

if get_option('default_library') == 'shared'
	args += [ '-DJPH_SHARED_LIBRARY' ]
endif

cpp = meson.get_compiler('cpp')

buildtype = get_option('buildtype')
build_debug = buildtype.contains('debug')
sanitize = get_option('b_sanitize')

broadphase_stats = get_option('broadphase_stats').disable_auto_if(not build_debug)
narrowphase_stats = get_option('narrowphase_stats').disable_auto_if(not build_debug)
debug_render = get_option('debug_render').disable_auto_if(not build_debug)
profile = get_option('profile').disable_auto_if(not build_debug)

disable_custom_allocator = get_option('disable_custom_allocator')
cross_platform_determinism = get_option('cross_platform_determinism')
viewer = get_option('viewer')

if sanitize.contains('address')
	disable_custom_allocator = true
	args += [ '-DJPH_DISABLE_TEMP_ALLOCATOR' ]
endif

if get_option('fpu_exceptions')
	args += [ '-DJPH_FLOATING_POINT_EXCEPTIONS_ENABLED' ]
endif

if disable_custom_allocator
	args += [ '-DJPH_DISABLE_TEMP_ALLOCATOR' ]
endif

if get_option('double_precision')
	args += [ '-DJPH_DOUBLE_PRECISION' ]
endif

if cross_platform_determinism
	args += [ '-DJPH_CROSS_PLATFORM_DETERMINISM' ]
endif

args += [ '-DJPH_OBJECT_LAYER_BITS=@0@'.format(get_option('object_layer_bits')) ]

if broadphase_stats.allowed()
	args += [ '-DJPH_TRACK_BROADPHASE_STATS' ]
endif

if narrowphase_stats.allowed()
	args += [ '-DJPH_TRACK_NARROWPHASE_STATS' ]
endif

if debug_render.allowed()
	args += [ '-DJPH_DEBUG_RENDERER' ]
endif

if profile.allowed()
	args += [ '-DJPH_PROFILE_ENABLED' ]
endif

if get_option('avx512')
	args += [ '-DJPH_USE_AVX512' ]
endif

if get_option('avx2')
	args += [ '-DJPH_USE_AVX2' ]
endif

if get_option('avx')
	args += [ '-DJPH_USE_AVX' ]
endif

if get_option('sse4_2')
	args += [ '-DJPH_USE_SSE4_2' ]
endif

if get_option('sse4_1')
	args += [ '-DJPH_USE_SSE4_1' ]
endif

if get_option('lzcnt')
	args += [ '-DJPH_USE_LZCNT' ]
	args += cpp.get_supported_arguments([ '-mlzcnt' ])
endif

if get_option('tzcnt')
	args += [ '-DJPH_USE_TZCNT' ]
	args += cpp.get_supported_arguments([ '-mbmi' ])
endif

if get_option('f16c')
	args += [ '-DJPH_USE_F16C' ]
	args += cpp.get_supported_arguments([ '-mf16c' ])
endif

if get_option('fmadd') and not cross_platform_determinism
	args += [ '-DJPH_USE_FMADD' ]
	args += cpp.get_supported_arguments([ '-mfma' ])
endif

if get_option('avx512')
	args += cpp.get_supported_arguments([
		'/arch:AVX512',
		'-mavx512f', '-mavx512vl',
		'-mavx512dq', '-mavx2',
		'-mbmi', '-mpopcnt',
		'-mlzcnt', '-mf16c'
	])
elif get_option('avx2')
	args += cpp.get_supported_arguments([
		'/arch:AVX2',
		'-mavx2',
		'-mbmi', '-mpopcnt',
		'-mlzcnt', '-mf16c'
	])
elif get_option('avx')
	args += cpp.get_supported_arguments([
		'/arch:AVX',
		'-mavx', '-mpopcnt',
	])
elif get_option('sse4.2')
	args += cpp.get_supported_arguments([
		'-msse4.2', '-mpopcnt'
	])
elif get_option('sse4.1')
	args += cpp.get_supported_arguments([
		'-msse4.1'
	])
endif

args += cpp.get_supported_arguments([ '-mfpmath=sse' ])

jolt = library('jolt', sources,
    include_directories : '.',
    extra_files : headers,
	cpp_pch : root/'Jolt.h',
	cpp_args : args,
	cpp_shared_args : [ '-DJPH_BUILD_SHARED_LIBRARY' ]
)

jolt_dep = declare_dependency(
    include_directories : '.',
    link_with : jolt,
	compile_args : args
)

meson.override_dependency('jolt', jolt_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('datadir') / 'licenses',
    rename : meson.project_name() + '.LICENSE.txt'
)
