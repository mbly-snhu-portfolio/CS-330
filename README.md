# CS-330 Final Project: 3D Scene with Mug and Saucer

## Project Description
This is the final project for CS-330 Computational Graphics and Visualization, demonstrating 3D scene creation with OpenGL. The scene features a mug with textured surfaces, dynamic lighting, and interactive camera controls.

## Controls
- **Keyboard**: WASD for movement, Q/E for up/down, P for perspective, O for orthographic
- **Mouse**: Move to look around, scroll to adjust speed

## Design and Development Reflection

### How I Approach Designing Software
I approach software design by first understanding the core requirements and breaking them down into manageable, modular components. I prioritize clean separation of concerns, ensuring each class or function has a single responsibility. For this project, I focused on creating a clear architecture where the ViewManager handles camera and input, while the SceneManager manages rendering and materials.

### New Design Skills Crafted
This project helped me craft skills in 3D scene composition, understanding how primitive shapes can be combined to create recognizable objects. I also developed a stronger sense of material properties and lighting design, learning how different shader parameters affect visual perception.

### Design Process Followed
My design process started with planning the basic scene composition and primitive requirements. I then implemented core functionality incrementally, testing each component before adding complexity. The process involved regular iteration based on visual feedback and performance considerations.

### Applying Tactics in Future Work
The modular approach and iterative design tactics I used here can be applied to future projects by maintaining clean separation between input, logic, and rendering. This makes code more maintainable and easier to extend with new features.

### How I Approach Developing Programs
I develop programs by starting with a clear understanding of the requirements, then building incrementally from working prototypes. I emphasize modular design and frequent testing to catch issues early. In this project, I built each component separately before integrating them.

### New Development Strategies Used
While working on the 3D scene, I used strategies like shader-based effects for lighting and textures, shadow mapping for realistic illumination, and smooth camera interpolation for better user experience. I also implemented depth buffer management for proper transparency rendering.

### Role of Iteration in Development
Iteration was crucial throughout development. I regularly refined object positioning, lighting parameters, and material properties based on visual feedback. Each milestone built upon the previous, allowing me to improve and polish the scene progressively.

### Evolution of Development Approach
My approach evolved from basic primitive rendering in early milestones to more sophisticated techniques like shadow mapping and material systems. I learned to balance visual quality with performance constraints and developed better debugging techniques for OpenGL applications.

### How Computer Science Helps Reach My Goals
Computer science provides the technical foundation needed to bring creative ideas to life through programming. The problem-solving skills and technical knowledge gained are directly applicable to both personal projects and professional development.

### New Knowledge from Computational Graphics for Education
Computational graphics and visualizations provide new knowledge in areas like shader programming, 3D mathematics, and visual design principles. These skills enhance understanding of computer graphics theory and prepare for advanced coursework in game development and computer vision.

### New Knowledge from Computational Graphics for Professional Pathways
Computational graphics and visualizations build a strong foundation for careers in game development, simulation, UI/UX design, and data visualization. The skills in shader programming, lighting, and 3D modeling directly apply to roles in video game studios, VR/AR development, and technical visualization for engineering firms. Understanding how to balance performance with visual quality is critical for creating polished applications that users expect in professional software. These techniques also enhance problem-solving for real-world applications like medical imaging or architectural modeling, where clear visual communication of complex data is essential.

## Getting Started
- Install Visual Studio and ensure OpenGL, GLEW, and GLFW are set up.
- Build and run the project to explore the 3D scene.
- Use the controls described above to navigate.

## Features
- Low-poly 3D models with textures and lighting
- Interactive camera with smooth controls
- Projection switching (perspective/orthographic)
- Spotlight shadows
- Ripple effect on liquid surface

## Project Structure
- `Source/`: Core C++ files
- `shaders/`: GLSL vertex and fragment shaders
- `textures/`: Image assets for materials
- `Debug/`: Build artifacts

## Dependencies
- OpenGL
- GLEW
- GLFW
- GLM

## License
This project is for educational purposes as part of CS-330 coursework. All assets are either created for this project or used under fair use for educational demonstration.
