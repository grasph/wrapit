# Examples of writing and reading a TTree

This directory show examples that use the `ROOT` library wrapper to write and read `TTrees`. 

💡 The examples do not handle all error conditions. `ROOT` `TTree` access mechanism uses heavily pointers and this a good case where further `Julia` code is needed to provide a friendly and safe (with respect to program crash) user interface.

## List of examples

### `write_tree1.jl`

  Fills a `TTree` with scalars. 

### `write_tree2.jl`

  Fills a `TTree` with events, whose object collections are stored as arrays of scalars inspired by the CMS NanoAOD format.

### `write_tree3.jl`

  Fills a `TTree` with events, whose object collectios are store as std vectors of scalars.

  Example of writing events with std vectors.

### `read_tree1.jl`

  Reads the tree produced by `write_tree_1.jl` using `TTree::GetEntry()`;

### `read_tree2.jl`

  Reads the tree produced by `write_tree_2.jl` using `UnROOT`.

### `read_tree3.jl`

  Reads the tree produced by `write_tree_2.jl` using `TTreeReader`.
  
## Examples in action

```julia
> julia write_tree1.jl
```

```
Creating a ROOT file with a TTree filled with scalars.

Writing value 1
Writing value 2
Writing value 3
Writing value 4
Writing value 5
Writing value 6
Writing value 7
Writing value 8
Writing value 9
Writing value 10
```

```julia
> julia read_tree1.jl
```

```
Reading back the file created with write_tree1.jl using TTree::GetEntry.

Read back value: 1
Read back value: 2
Read back value: 3
Read back value: 4
Read back value: 5
Read back value: 6
Read back value: 7
Read back value: 8
Read back value: 9
Read back value: 10
```

```julia
> julia write_tree2.jl
```

```
Creating a ROOT file with a TTree filled with  arrays.

Fill tree with an event containing 2 muons.
Fill tree with an event containing 3 muons.
Fill tree with an event containing 10 muons.
Fill tree with an event containing 8 muons.
Fill tree with an event containing 1 muons.
Fill tree with an event containing 6 muons.
Fill tree with an event containing 4 muons.
Fill tree with an event containing 2 muons.
Fill tree with an event containing 5 muons.
Fill tree with an event containing 10 muons.

```

```julia
> julia read_tree2.jl
```

```
Reading back the file created with write_tree2.jl using LazyTree of UnROOT.

 Row │ nMuon  Muon_phi[nMuon]       Muon_pt[nMuon]        Muon_eta[nMuon]
     │ Int32  SubArray{Float3       SubArray{Float3       SubArray{Float3
─────┼─────────────────────────────────────────────────────────────────────────
 1   │ 2      [1.64, 4.72]          [47.6, 15.2]          [0.508, 3.76]
 2   │ 3      [1.34, 3.06, 6.08]    [68.2, 12.6, 92.8]    [-0.788, -3.44, 0.17
 3   │ 10     [3.22, 2.66, 5.1, 3.  [56.3, 74.2, 46.2, 1  [-4.94, -0.965, 0.53
 4   │ 8      [3.18, 0.791, 5.83,   [48.5, 28.1, 98.6, 3  [3.3, -0.516, 2.34,
 5   │ 1      [0.81]                [71.9]                [1.63]
 6   │ 6      [1.78, 1.72, 5.96, 2  [38.2, 60.4, 43.5, 4  [-3.9, 3.43, 4.62, 0
 7   │ 4      [4.58, 6.05, 4.29, 3  [89.1, 12.0, 6.68, 4  [0.815, 2.79, 0.736,
 8   │ 2      [1.91, 3.01]          [30.7, 30.8]          [-1.87, -2.01]
 9   │ 5      [6.01, 4.42, 5.42, 0  [42.7, 35.4, 65.9, 6  [-4.57, 2.23, 1.32,
 10  │ 10     [3.07, 5.79, 0.16, 5  [31.2, 61.7, 0.254,   [1.31, -4.14, -1.44,


```

```julia
> julia write_tree3.jl
```

```
Creating a ROOT file with a TTree filled with std vectors.

Fill tree with an event containing 7 muons.
Fill tree with an event containing 10 muons.
Fill tree with an event containing 10 muons.
Fill tree with an event containing 5 muons.
Fill tree with an event containing 9 muons.
Fill tree with an event containing 9 muons.
Fill tree with an event containing 10 muons.
Fill tree with an event containing 4 muons.
Fill tree with an event containing 9 muons.
Fill tree with an event containing 3 muons.

```

```julia
> julia read_tree3.jl
```

```
Reading back the file created with write_tree3.jl using TTreeReader.

Event 1
Muon multiplicity: 7
Muon pt: Float32[99.55316, 16.922623, 31.833536, 77.957344, 77.65671, 38.46408, 37.92194]
Muon eta: Float32[-3.8800058, 3.8216572, 2.3580778, 4.236388, 0.6129842, 3.2124858, 4.7774787]
Muon phi: Float32[3.593313, 3.392881, 0.63297457, 0.6154244, 5.0248528, 6.270981, 5.264125]

Event 2
Muon multiplicity: 10
Muon pt: Float32[91.0112, 71.14534, 80.26713, 94.39505, 98.83605, 7.2041807, 8.665472, 70.25187, 79.03746, 46.448433]
Muon eta: Float32[-0.7017298, -1.2581806, 1.8125896, 2.3257022, -1.9423141, -0.89880705, 4.69867, 2.0136452, 1.3956566, 0.6887522]
Muon phi: Float32[3.591252, 4.4214025, 3.7171175, 2.317142, 2.351868, 1.6154821, 2.174602, 1.8317109, 1.4419165, 4.783898]

Event 3
Muon multiplicity: 10
Muon pt: Float32[22.408045, 92.30349, 35.809196, 31.32763, 95.96019, 34.21476, 99.44961, 98.05886, 62.511562, 12.559271]
Muon eta: Float32[2.0520778, 3.6122859, 4.3450046, 2.1695108, -3.1774178, -0.96796465, -1.00312, -1.3053713, -3.5725317, -4.743335]
Muon phi: Float32[5.099305, 2.665869, 1.7780039, 2.4221466, 2.960289, 1.2408991, 0.89774805, 2.2342472, 0.18622024, 1.069258]

Event 4
Muon multiplicity: 5
Muon pt: Float32[71.00361, 50.209785, 18.546427, 51.01913, 28.637249]
Muon eta: Float32[3.1980813, 0.67387676, 0.5545683, 1.0940714, 1.8726873]
Muon phi: Float32[4.1700196, 5.9657907, 0.28005335, 5.191479, 3.073251]

Event 5
Muon multiplicity: 9
Muon pt: Float32[50.344078, 71.2709, 12.999368, 0.2846539, 29.36536, 49.31215, 29.124767, 9.871805, 90.116844]
Muon eta: Float32[-3.775138, -4.4510612, -2.5394683, -1.1612997, -0.47946692, 3.7213402, -2.5396585, 0.9428501, -2.8013945]
Muon phi: Float32[3.9734166, 0.95552844, 2.2504861, 4.431298, 3.4690332, 5.4882507, 3.6831775, 3.51927, 4.730269]

Event 6
Muon multiplicity: 9
Muon pt: Float32[86.17218, 95.33835, 76.35522, 97.47732, 43.34685, 38.71627, 48.277813, 63.819294, 88.54618]
Muon eta: Float32[0.48927498, 0.3876934, -0.36594486, 4.6575165, 1.7535, 2.8994024, -1.7393632, -3.3755026, -0.13483429]
Muon phi: Float32[1.2089372, 2.7121181, 6.1601677, 0.054604247, 1.2415103, 5.385047, 0.1752794, 1.9181306, 5.188641]

Event 7
Muon multiplicity: 10
Muon pt: Float32[66.17291, 78.74383, 37.03076, 13.662254, 68.711494, 92.80245, 94.44082, 84.13793, 52.68166, 63.058178]
Muon eta: Float32[3.401904, -1.9566846, 3.6424792, 0.6648593, 0.15331507, 2.664116, -3.841136, -3.9262295, 1.4249067, 2.650863]
Muon phi: Float32[5.01442, 0.7198939, 3.910881, 1.9486507, 5.8971157, 2.1646726, 6.0961676, 4.9805145, 1.0151991, 0.734682]

Event 8
Muon multiplicity: 4
Muon pt: Float32[53.603916, 72.33049, 13.469065, 88.95076]
Muon eta: Float32[-3.145523, -2.6973748, -3.3713217, 1.0014868]
Muon phi: Float32[5.1253433, 2.6984644, 3.1373065, 3.2159207]

Event 9
Muon multiplicity: 9
Muon pt: Float32[75.21912, 83.52651, 30.86543, 95.58508, 15.835751, 94.75457, 17.469538, 39.655323, 70.25344]
Muon eta: Float32[4.597922, -1.688292, -1.7719603, -3.812788, 2.3028052, 1.5675688, 3.220235, 4.716823, -0.054864407]
Muon phi: Float32[4.639649, 4.39589, 6.151756, 1.9772443, 3.7280943, 2.7042751, 0.91994244, 6.2789087, 0.27550048]

Event 10
Muon multiplicity: 3
Muon pt: Float32[64.674965, 66.53387, 64.06248]
Muon eta: Float32[-4.0431795, 0.86733294, 0.9855647]
Muon phi: Float32[5.3794937, 5.2301254, 6.0675263]
```
