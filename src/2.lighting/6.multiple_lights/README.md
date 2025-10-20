## Concept
I wanted to make a growing L system fractual tree with procedural generation with real-time 3D rendering

## Main features
- **L-system Generation:** It uses a simple "F[+F][-F]" rule to programmatically generate a complex, self-similar binary tree structure as a string.
- **Turtle Graphics Interpretation:** A "turtle" algorithm translates this string into 3D geometry, controlling its position, direction, and local coordinate system (up, right) for accurate branching and rotations.
- **Animatable Growth:** The tree's appearance is animated over time, allowing branches and segments to sequentially "grow" into place from the base outwards.
- **Dynamic Lighting:** The scene is lit by a directional light, a spotlight controlled by the camera, and multiple firefly point lights that orbit around the tree, casting dynamic illumination.
- **PBR-like Materials (Simplified):** It uses diffuse and specular texture maps to give the tree a more realistic, wood-like appearance, interacting with the various light sources.
- **Interactive Camera:** The user can navigate the scene freely using a first-person camera, providing different perspectives of the growing, illuminated tree.

## Credits
1. [Wood texture](https://ambientcg.com/view?id=Wood047)
2. [L system explaination and formula](https://graphicmaths.com/fractals/l-systems/l-system-trees/)
