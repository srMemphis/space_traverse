# space_traverse

Simulates an electron beam passing through a geometry with dielectric bodies that accumulate charge. The field is recomputed as charge builds up, so the beam and the charge evolve together.

## Language

### Geometry

**Volume**:
A named part of the geometry with one role: Target, Mask, Detector or World.
_Avoid_: region, physical group, phys name

**Target**:
A dielectric Volume that accumulates charge from the electrons it absorbs.
_Avoid_: Tube, Plane, Capillary, sample, specimen

**Mask**:
A grounded conductor Volume that absorbs electrons without accumulating charge.
_Avoid_: Holder, Foil, Metal, aperture

**Detector**:
A sensitive Volume that records a Hit for every Track entering it and does not affect the Field. A Detector either stops the Track or lets it pass.
_Avoid_: screen, collector, plate

**World**:
The vacuum Volume surrounding all others, through which Tracks travel.
_Avoid_: Air, environment, space

**Boundary**:
The closed surface of a Volume, named "<Volume> Boundary".
_Avoid_: surface

**Grounded boundary**:
A Boundary held at 0 V: the outer Boundary of the World and the Boundary of every Mask.
_Avoid_: Dirichlet boundary

**Material**:
The physical properties of a Volume: its relative permittivity and its Secondary emission yield.
_Avoid_: medium

### Mesh

**Mesh**:
The partition of the geometry into Elements on which the Potential is solved.
_Avoid_: grid

**Element**:
One tetrahedron of the Mesh. Every Element belongs to exactly one Volume.
_Avoid_: tetra, cell

**Mesh bounds**:
The smallest axis-aligned box that contains the whole Mesh.
_Avoid_: bounding box

**Narrow bounds**:
The Mesh bounds shrunk by 0.1% about their centre, so a point inside them lies safely inside the Mesh.
_Avoid_: tight box

### Beam and tracks

**Beam**:
The source of Primary electrons: a disk of Beam radius centred on the Beam origin and pointed at the Aim point.

**Beam origin**:
The centre of the Beam disk.
_Avoid_: Pos, center position

**Aim point**:
The point the Beam axis passes through, which sets the Beam direction.
_Avoid_: Stare, target point

**Beam radius**:
The radius of the Beam disk.

**Divergence**:
The full opening angle of the cone of starting directions around the Beam axis.

**Beam energy**:
The initial kinetic energy of a Primary electron, in eV.

**Beam current**:
The signed current the Beam carries; negative for electrons.

**Primary electron**:
An electron emitted by the Beam, as opposed to a secondary electron.
_Avoid_: particle

**Start point**:
The point on the Beam disk where a Track begins.
_Avoid_: primary position

**Track**:
The path of one representative Primary electron through the World during one Iteration.
_Avoid_: particle, ray, trajectory

**Track charge**:
The charge one Track represents: Beam current × Iteration time / number of Tracks.

**Step**:
One advance of a Track along its path.
_Avoid_: verlet step

**Step length**:
The distance a Track covers in one Step.
_Avoid_: step size, dt

**Landing energy**:
The kinetic energy of a Track at its Absorption or Hit, in eV.
_Avoid_: impact energy, hit energy

### Track ends

**Absorption**:
A Track ending inside a Target or a Mask.
_Avoid_: kill, impact

**Escape**:
A Track leaving the World.
_Avoid_: lost track

**Hit**:
The record a Detector keeps of a Track that entered it: position and Landing energy.
_Avoid_: impact; never use for an Absorption

### Charging

**Iteration**:
One charging cycle: solve the Potential, trace every Track through the Field, deposit the absorbed charge.
_Avoid_: time step, frame, epoch

**Iteration time**:
The physical time one Iteration represents.
_Avoid_: deltaTime, time step, dt

**Potential**:
The electric potential across all Volumes during one Iteration.
_Avoid_: solution, u, fi

**Field**:
The electric field: the negative gradient of the Potential.

**Secondary emission yield**:
The mean number of electrons leaving a Material per absorbed electron (σ), a function of Landing energy.
_Avoid_: secondary coefficient

**Crossover energy**:
A Landing energy at which the Secondary emission yield equals 1.
_Avoid_: equilibrium energy

**Charge deposition**:
Adding (1 − σ) × Track charge to a Target at the point of Absorption.
_Avoid_: element charge

**Charge density**:
The charge accumulated per unit volume in a Target.
_Avoid_: surface charge, element charge

**Charging equilibrium**:
The state in which a Target's own charge slows incoming electrons until their Landing energy reaches a Crossover energy. From then on, secondary electrons carry away as much charge as the Tracks bring, and the Target's charge stops changing.

### Observables

**Transmitted current**:
The current carried per Iteration by the Tracks that produce a Hit.
_Avoid_: detector current

**Beam profile**:
The distribution of Hit positions across the Beam.

### Runs

**Scenario**:
The geometry and parameters of one problem.
_Avoid_: config, case

**Run**:
One execution of a Scenario: a whole beam transmission experiment over many Iterations, with the Field recomputed every Iteration as Targets charge. Unlike a GEANT4 run, the Field is not fixed.
_Avoid_: simulation

**Snapshot**:
The saved state at one Iteration: Potential, Tracks, Charge density and Hits.
_Avoid_: solution, solution file
