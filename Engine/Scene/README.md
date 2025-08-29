# Scene module

`vx-scene` is a standalone C++20 library. Include `<vx/Scene/Scene.h>` and link the target. Rendering and asset loading remain application responsibilities; sprites describe asset paths, source frames, dimensions, tint, and visibility.

## Coordinate and update conventions

- Positive x points right and positive y points down. `Rect` uses center x/y and positive width/height.
- Collider bounds are local offsets and dimensions. Position and scale affect bounds; rotation is visual and does not rotate colliders.
- Call `Scene::update` with a finite `dt` from 0 through 1 second. A fixed timestep such as 1/120 second provides consistent results.
- Active entities run scripts in ascending entity ID order. `onStart` runs once before the first update. Entities created during callbacks begin updating on the next tick. A callback may delete itself or another entity.
- `destroyEntity` invalidates the entity before calling `onDestroy`. `clear` destroys the current snapshot; entities spawned by destroy callbacks survive. Destruction of the C++ scene object itself does not invoke application callbacks.
- References and pointers remain valid while their entity exists. A successful load replaces all entities and invalidates previous entity references. Entity IDs must not be edited directly.

## Physics scope

Dynamic bodies use semi-implicit Euler gravity. Enabled, non-trigger AABBs sweep against static entities and solid tile rectangles, preventing fast movement from tunneling through static walls. Collision response retains remaining movement time, allows sliding, and supports restitution from 0 to 1. Up to eight impacts are resolved per tick; any remaining time is discarded at that bound. Initial penetration receives up to four correction passes.

Dynamic body pairs receive a discrete equal-mass AABB separation and impulse. This is suitable for simple games; it is not continuous dynamic-pair physics, rotational physics, friction, joints, or a rigid-body stacking solver. Use `sweepSegment` or `sweepAabb` explicitly for fast projectiles against moving gameplay targets.

Trigger contacts report overlap or swept contact along actual movement without blocking. Dynamic-pair trigger detection is discrete. Contacts last until the next update. A contact's second ID of zero identifies a tile wall. Adjacent solid tiles merge horizontally within each row.

## Scene data

`save(std::ostream&)` and `load(std::istream&, std::string*)` use versioned `VXN_SCENE 1` text. The data includes gravity, stable IDs, names, transforms, sprite data, rigid bodies, collider flags, script binding keys, and solid tilemaps. It uses the classic locale and preserves float round trips.

The loader validates version, record tags, IDs, booleans, finite geometry, positive dimensions, restitution, colors, and resource/name lengths. It rejects duplicate IDs, truncated input, invalid cells, and trailing data. Limits are 100,000 entities, 1,024 tilemaps, 1,000,000 total tile cells, and 4,096 bytes per name or resource key. A failed load preserves the original scene and callbacks. No script code executes while loading; successful loads retain binding keys and require the application to reattach callbacks. Saving builds the document before writing, so invalid component data does not emit a partial document; applications should still use a temporary file and rename for atomic disk replacement.

## Utility behavior

`Animation` selects looping or clamped source frames. `Tween` supports linear, smoothstep, and quadratic ease-in-out position interpolation. `TimerQueue` runs callbacks in timer-ID order, supports callback cancellation and creation, and fires repeating timers at most once per update while keeping interval phase. `ParticleEmitter` has a fixed maximum capacity, deterministic seeded velocity spread, acceleration, and lifetime reclamation. `SceneManager` holds named scenes, updates one active scene, and safely supports removing the active scene from its own callback.
