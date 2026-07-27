# AGENTS.md — Source/Core/Physics/

The engine's physics integration. The **Physics system wraps the [Jolt Physics](https://github.com/jrouwe/JoltPhysics)
library** — an open-source, multi-threaded, real-time rigid-body physics engine — exposing it to
the rest of GiiGa as a singleton that manages body creation/removal, simulation stepping, collision
layers, contact callbacks, and shape casting.

In the `GiiGa` namespace.

## Files

| File | Class(es) | Purpose |
|------|-----------|---------|
| `PhysicsSystem.h` | `class PhysicsSystem` (singleton), `GiiGaContactListener`, helpers | Header-only singleton facade around `JPH::PhysicsSystem`. Configures Jolt (allocator, factory, trace/assert callbacks), defines object & broadphase collision layers (`NON_MOVING`, `MOVING`, `TRIGGER`) and their filter rules, registers `ICollision` components as Jolt bodies (box/sphere shapes) bound to engine transforms, steps the simulation each frame, and dispatches queued contact events (added/persisted/removed) back to engine-side `ICollision` components. Also provides a sphere `ShapeCast` query. |

### Key API on `PhysicsSystem`

- `GetInstance()` — singleton accessor.
- `Initialize` / `Destroy` — Jolt setup/teardown.
- `BeginPlay` / `EndPlay` — physics world lifecycle tied to `World` Play/Edit state.
- `Simulate(dt)` — advances the simulation one frame and flushes contact-event queues.
- `RegisterCollision` / `UnRegisterCollision` — add/remove an `ICollision` component as a body.
- `ShapeCast(...)` — sphere sweep returning a `ShapeCastResult` (the hit `ICollision`).
- `GetJoltPhysicsSystem` / `GetBodyInterface` / `GetCollisionByBody` — low-level access.

### Helper types (same file)

- `ObjectLayerPairFilterImpl`, `BPLayerInterfaceImpl`, `ObjectVsBroadPhaseLayerFilterImpl`,
  `MyBodyActivationListener`, `CastShapeCollector` — Jolt configuration/strategy objects.
- `struct ShapeCastResult` — hit result holding the struck `ICollision`.
- Namespaces `Layers` / `BroadPhaseLayers` — collision-layer constants.
- `TraceImpl` / `AssertFailedImpl` — Jolt logging hooks.

## Integration points

- Engine components implement `ICollision` ([`../../ICollision.h`](../../ICollision.h)) and the
  concrete `CollisionComponent` ([`../../../Components/CollisionComponent.h`](../../../Components/CollisionComponent.h)).
- `EditorEngine`/`World` call `PhysicsSystem::Simulate` during the tick and `BeginPlay`/`EndPlay`
  on Play/Edit transitions.
