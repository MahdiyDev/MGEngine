# TODO

- [x] draw line and triangle together.
- [x] draw check limit
- [x] set cursor hidden. (Hide/Show/Enable/DisableCursor, Mge_ToggleCursor, IsCursorHidden; TAB-bound in main.c)
- [x] add gizmo (2D, 3D)
- [x] lighting: ambient + diffuse + specular (Phong), Material on Object, Light as a scene entity
- [x] material maps: MaterialMap (texture + color + value) slots on Material (diffuse / specular)
- [x] light types: directional / point / spot (+ flashlight, soft cone edges), multi-light pass
- [x] Mesh: vertices + indices + textures, own VAO/VBO/EBO (Mge_MakeMesh/Upload/Draw/Unload)
- [x] Model: Mge_LoadModel via Assimp (glTF/OBJ/FBX) -> meshes + directory + bbox; node transforms baked in
- [ ] batch rendering
- [ ] multiple vertex rendering