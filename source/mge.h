#pragma once

#include "mge_config.h"
#include "mge_math.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MGE_VERSION "v0.0.1"

#define CLITERAL(type) (type)

#ifndef MGE_CULL_DISTANCE_NEAR
	#define MGE_CULL_DISTANCE_NEAR		0.01
#endif
#ifndef MGE_CULL_DISTANCE_FAR
	#define MGE_CULL_DISTANCE_FAR		1000.0
#endif
#ifndef MAX_KEY_PRESSED_QUEUE
	#define MAX_KEY_PRESSED_QUEUE  16  // Maximum number of keys in the key input queue
#endif
#ifndef MAX_CHAR_PRESSED_QUEUE
	#define MAX_CHAR_PRESSED_QUEUE 16  // Maximum number of characters in the char input queue
#endif

void Trace_Log(int logType, const char* text, ...);
typedef void (*Trace_Log_Callback)(int logLevel, const char* text, va_list args); // Logging: Redirect trace log messages
typedef enum {
	LOG_ALL = 0,		// Display all logs
	LOG_TRACE,			// Trace logging, intended for internal use only
	LOG_DEBUG,			// Debug logging, used for internal debugging, it should be disabled on release builds
	LOG_INFO,			// Info logging, used for program execution info
	LOG_WARNING,		// Warning logging, used on recoverable failures
	LOG_ERROR,			// Error logging, used on unrecoverable failures
	LOG_FATAL,			// Fatal logging, used to abort program: exit(EXIT_FAILURE)
	LOG_NONE			// Disable logging
} TraceLogLevel;

typedef enum {
	KEY_NULL				= 0,		// Key: NULL, used for no key pressed
	// Alphanumeric keys
	KEY_APOSTROPHE	  		= 39,	   // Key: '
	KEY_COMMA		   		= 44,	   // Key: ,
	KEY_MINUS		   		= 45,	   // Key: -
	KEY_PERIOD		  		= 46,	   // Key: .
	KEY_SLASH		   		= 47,	   // Key: /
	KEY_ZERO				= 48,	   // Key: 0
	KEY_ONE			 		= 49,	   // Key: 1
	KEY_TWO			 		= 50,	   // Key: 2
	KEY_THREE		   		= 51,	   // Key: 3
	KEY_FOUR				= 52,	   // Key: 4
	KEY_FIVE				= 53,	   // Key: 5
	KEY_SIX			 		= 54,	   // Key: 6
	KEY_SEVEN		   		= 55,	   // Key: 7
	KEY_EIGHT		   		= 56,	   // Key: 8
	KEY_NINE				= 57,	   // Key: 9
	KEY_SEMICOLON	   		= 59,	   // Key: ;
	KEY_EQUAL		   		= 61,	   // Key: =
	KEY_A			   		= 65,	   // Key: A | a
	KEY_B			   		= 66,	   // Key: B | b
	KEY_C			   		= 67,	   // Key: C | c
	KEY_D			   		= 68,	   // Key: D | d
	KEY_E			   		= 69,	   // Key: E | e
	KEY_F			   		= 70,	   // Key: F | f
	KEY_G			   		= 71,	   // Key: G | g
	KEY_H			   		= 72,	   // Key: H | h
	KEY_I			   		= 73,	   // Key: I | i
	KEY_J			   		= 74,	   // Key: J | j
	KEY_K			   		= 75,	   // Key: K | k
	KEY_L			   		= 76,	   // Key: L | l
	KEY_M			   		= 77,	   // Key: M | m
	KEY_N			   		= 78,	   // Key: N | n
	KEY_O			   		= 79,	   // Key: O | o
	KEY_P			   		= 80,	   // Key: P | p
	KEY_Q			   		= 81,	   // Key: Q | q
	KEY_R			   		= 82,	   // Key: R | r
	KEY_S			   		= 83,	   // Key: S | s
	KEY_T			   		= 84,	   // Key: T | t
	KEY_U			   		= 85,	   // Key: U | u
	KEY_V			   		= 86,	   // Key: V | v
	KEY_W			   		= 87,	   // Key: W | w
	KEY_X			   		= 88,	   // Key: X | x
	KEY_Y			   		= 89,	   // Key: Y | y
	KEY_Z			   		= 90,	   // Key: Z | z
	KEY_LEFT_BRACKET		= 91,	   // Key: [
	KEY_BACKSLASH	   		= 92,	   // Key: '\'
	KEY_RIGHT_BRACKET   	= 93,	   // Key: ]
	KEY_GRAVE		   		= 96,	   // Key: `
	// Function keys
	KEY_SPACE		   		= 32,	   // Key: Space
	KEY_ESCAPE		  		= 256,	  // Key: Esc
	KEY_ENTER		   		= 257,	  // Key: Enter
	KEY_TAB			 		= 258,	  // Key: Tab
	KEY_BACKSPACE	   		= 259,	  // Key: Backspace
	KEY_INSERT		  		= 260,	  // Key: Ins
	KEY_DELETE		  		= 261,	  // Key: Del
	KEY_RIGHT		   		= 262,	  // Key: Cursor right
	KEY_LEFT				= 263,	  // Key: Cursor left
	KEY_DOWN				= 264,	  // Key: Cursor down
	KEY_UP			  		= 265,	  // Key: Cursor up
	KEY_PAGE_UP		 		= 266,	  // Key: Page up
	KEY_PAGE_DOWN	   		= 267,	  // Key: Page down
	KEY_HOME				= 268,	  // Key: Home
	KEY_END			 		= 269,	  // Key: End
	KEY_CAPS_LOCK	   		= 280,	  // Key: Caps lock
	KEY_SCROLL_LOCK	 		= 281,	  // Key: Scroll down
	KEY_NUM_LOCK			= 282,	  // Key: Num lock
	KEY_PRINT_SCREEN		= 283,	  // Key: Print screen
	KEY_PAUSE		   		= 284,	  // Key: Pause
	KEY_F1			  		= 290,	  // Key: F1
	KEY_F2			  		= 291,	  // Key: F2
	KEY_F3			  		= 292,	  // Key: F3
	KEY_F4			  		= 293,	  // Key: F4
	KEY_F5			  		= 294,	  // Key: F5
	KEY_F6			  		= 295,	  // Key: F6
	KEY_F7			  		= 296,	  // Key: F7
	KEY_F8			  		= 297,	  // Key: F8
	KEY_F9			  		= 298,	  // Key: F9
	KEY_F10			 		= 299,	  // Key: F10
	KEY_F11			 		= 300,	  // Key: F11
	KEY_F12			 		= 301,	  // Key: F12
	KEY_LEFT_SHIFT	  		= 340,	  // Key: Shift left
	KEY_LEFT_CONTROL		= 341,	  // Key: Control left
	KEY_LEFT_ALT			= 342,	  // Key: Alt left
	KEY_LEFT_SUPER	  		= 343,	  // Key: Super left
	KEY_RIGHT_SHIFT	 		= 344,	  // Key: Shift right
	KEY_RIGHT_CONTROL   	= 345,	  // Key: Control right
	KEY_RIGHT_ALT	   		= 346,	  // Key: Alt right
	KEY_RIGHT_SUPER	 		= 347,	  // Key: Super right
	KEY_KB_MENU		 		= 348,	  // Key: KB menu
	// Keypad keys
	KEY_KP_0				= 320,	  // Key: Keypad 0
	KEY_KP_1				= 321,	  // Key: Keypad 1
	KEY_KP_2				= 322,	  // Key: Keypad 2
	KEY_KP_3				= 323,	  // Key: Keypad 3
	KEY_KP_4				= 324,	  // Key: Keypad 4
	KEY_KP_5				= 325,	  // Key: Keypad 5
	KEY_KP_6				= 326,	  // Key: Keypad 6
	KEY_KP_7				= 327,	  // Key: Keypad 7
	KEY_KP_8				= 328,	  // Key: Keypad 8
	KEY_KP_9				= 329,	  // Key: Keypad 9
	KEY_KP_DECIMAL	  		= 330,	  // Key: Keypad .
	KEY_KP_DIVIDE	   		= 331,	  // Key: Keypad /
	KEY_KP_MULTIPLY	 		= 332,	  // Key: Keypad *
	KEY_KP_SUBTRACT	 		= 333,	  // Key: Keypad -
	KEY_KP_ADD		  		= 334,	  // Key: Keypad +
	KEY_KP_ENTER			= 335,	  // Key: Keypad Enter
	KEY_KP_EQUAL			= 336,	  // Key: Keypad =
	// Android key buttons
	KEY_BACK				= 4,      // Key: Android back button
	KEY_MENU				= 5,      // Key: Android menu button
	KEY_VOLUME_UP	   		= 24,     // Key: Android volume up button
	KEY_VOLUME_DOWN	 		= 25      // Key: Android volume down button
} KeyboardKey;

typedef enum {
	PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1, // 8 bit per pixel (no alpha)
	PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,    // 8*2 bpp (2 channels)
	PIXELFORMAT_UNCOMPRESSED_R5G6B5,        // 16 bpp
	PIXELFORMAT_UNCOMPRESSED_R8G8B8,        // 24 bpp
	PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,      // 16 bpp (1 bit alpha)
	PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,      // 16 bpp (4 bit alpha)
	PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,      // 32 bpp
	PIXELFORMAT_UNCOMPRESSED_R32,           // 32 bpp (1 channel - float)
	PIXELFORMAT_UNCOMPRESSED_R32G32B32,     // 32*3 bpp (3 channels - float)
	PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,  // 32*4 bpp (4 channels - float)
	PIXELFORMAT_UNCOMPRESSED_R16,           // 16 bpp (1 channel - half float)
	PIXELFORMAT_UNCOMPRESSED_R16G16B16,     // 16*3 bpp (3 channels - half float)
	PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,  // 16*4 bpp (4 channels - half float)
	PIXELFORMAT_COMPRESSED_DXT1_RGB,        // 4 bpp (no alpha)
	PIXELFORMAT_COMPRESSED_DXT1_RGBA,       // 4 bpp (1 bit alpha)
	PIXELFORMAT_COMPRESSED_DXT3_RGBA,       // 8 bpp
	PIXELFORMAT_COMPRESSED_DXT5_RGBA,       // 8 bpp
	PIXELFORMAT_COMPRESSED_ETC1_RGB,        // 4 bpp
	PIXELFORMAT_COMPRESSED_ETC2_RGB,        // 4 bpp
	PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,   // 8 bpp
	PIXELFORMAT_COMPRESSED_PVRT_RGB,        // 4 bpp
	PIXELFORMAT_COMPRESSED_PVRT_RGBA,       // 4 bpp
	PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,   // 8 bpp
	PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA    // 2 bpp
} PixelFormat;

// Camera projection
typedef enum {
	CAMERA_PERSPECTIVE = 0,  // Perspective projection
	CAMERA_ORTHOGRAPHIC	     // Orthographic projection
} CameraProjection;

typedef struct Color {
	unsigned char r;	// Color red value
	unsigned char g;	// Color green value
	unsigned char b;	// Color blue value
	unsigned char a;	// Color alpha value
} Color;

typedef struct Rectangle {
	float x;			// Rectangle top-left corner position x
	float y;			// Rectangle top-left corner position y
	float width;		// Rectangle width
	float height;		// Rectangle height
} Rectangle;

typedef struct Shader {
	unsigned int id;        // Shader program id
	int *locs;              // Shader locations array (MGE_MAX_SHADER_LOCATIONS)
} Shader;

// Texture, tex data stored in GPU memory (VRAM)
typedef struct Texture {
	unsigned int id;        // OpenGL texture id
	int width;              // Texture base width
	int height;             // Texture base height
	int mipmaps;            // Mipmap levels, 1 by default
	int format;             // Data format (PixelFormat type)
} Texture;

// Texture2D, same as Texture
typedef Texture Texture2D;

// Image, pixel data stored in CPU memory (RAM)
typedef struct Image {
    void *data;             // Image raw data
    int width;              // Image base width
    int height;             // Image base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
} Image;

#define RED			CLITERAL(Color) { 255, 0, 0, 255 }
#define GREEN		CLITERAL(Color) { 0, 255, 0, 255 }
#define BLUE		CLITERAL(Color) { 0, 0, 255, 255 }
#define DARKGREEN	CLITERAL(Color) { 51, 77, 77, 255 }
#define BLACK		CLITERAL(Color) { 0, 0, 0, 255 }
#define GRAY		CLITERAL(Color) { 130, 130, 130, 255 }
#define LIGHTGRAY	CLITERAL(Color) { 200, 200, 200, 255 }
#define DARKGRAY	CLITERAL(Color) { 80, 80, 80, 255 }
#define WHITE		CLITERAL(Color) { 255, 255, 255, 255 }
#define YELLOW		CLITERAL(Color) { 255, 255, 0, 255 }

typedef struct Camera3D {
	Vector3 position;	   // Camera position
	Vector3 target;		 // Camera target it looks-at
	Vector3 up;			 // Camera up vector (rotation over its axis)
	float fovy;			 // Camera field-of-view aperture in Y (degrees) in perspective, used as near plane width in orthographic
	int projection;		 // Camera projection: CAMERA_PERSPECTIVE or CAMERA_ORTHOGRAPHIC
} Camera3D;

#ifdef CORE_INCLUDE
typedef struct {
	int x;
	int y;
} Point;
typedef struct {
	uint32_t width;
	uint32_t height;
} Size;

#define MAX_KEYBOARD_KEYS 512
#define MAX_MOUSE_BUTTONS 8

typedef struct CoreData {
	struct {
		const char* title;
		Size display;
		Size screen;
		Size render;
		bool ready;
		bool shouldClose;
	} Window;

	struct {
		double frame;
		unsigned int frameCounter;
		unsigned long long int base;
		double current;
		double draw;
		double previous;
		double target;
		double update;
		double deltaTime;
		double lastFrame;
	} Time;

	struct {
		struct {
			char currentKeyState[MAX_KEYBOARD_KEYS];	  // Registers current frame key state
			char previousKeyState[MAX_KEYBOARD_KEYS];	 // Registers previous frame key state

			// NOTE: Since key press logic involves comparing prev vs cur key state, we need to handle key repeats specially
			char keyRepeatInFrame[MAX_KEYBOARD_KEYS];	 // Registers key repeats for current frame.

			int keyPressedQueue[MAX_KEY_PRESSED_QUEUE];   // Input keys queue
			int keyPressedQueueCount;					 // Input keys queue count

			int charPressedQueue[MAX_CHAR_PRESSED_QUEUE]; // Input characters queue (unicode)
			int charPressedQueueCount;					// Input characters queue count
		} Keyboard;

		struct {
			Vector2 currentPosition;
			Vector2 previousPosition;
			Vector2 offset;
			Vector2 scale;
			bool cursorHidden;
			char currentButtonState[MAX_MOUSE_BUTTONS];
			char previousButtonState[MAX_MOUSE_BUTTONS];
		} Mouse;
	} Input;
} CoreData;
#endif

// Mouse buttons
typedef enum {
	MOUSE_BUTTON_LEFT   = 0,
	MOUSE_BUTTON_RIGHT  = 1,
	MOUSE_BUTTON_MIDDLE = 2
} MouseButton;

// A movable scene object -- a rectangle in 2D, an axis-aligned box in 3D.
typedef enum {
	OBJECT_2D = 0,
	OBJECT_3D
} ObjectKind;

// --- Lighting (Phong: ambient + diffuse + specular) ---
//
// A `Light` is a scene entity (its position, colour and per-term strengths).
// A `Material` is the *surface* response and is attached to an Object; several
// objects with different materials can be lit by the same light.

// Material map slots. A `Material` holds one `MaterialMap` per slot; each map
// pairs a texture with a colour and a scalar `value` whose meaning depends on
// the slot:
//
//   MATERIAL_MAP_DIFFUSE   .texture  albedo map (falls back to a white 1x1
//                                    texture when .texture.id == 0)
//                          .color    albedo tint, multiplies the texture
//                          .value    unused
//   MATERIAL_MAP_SPECULAR  .texture  unused
//                          .color    unused (reserved for a coloured highlight)
//                          .value    specular strength multiplier (1 = as-is,
//                                    0 = no highlight)
typedef enum {
	MATERIAL_MAP_DIFFUSE = 0,   // base colour / albedo
	MATERIAL_MAP_SPECULAR,      // specular highlight strength
	MATERIAL_MAP_COUNT
} MaterialMapIndex;

typedef struct MaterialMap {
	Texture2D texture;  // map texture (id 0 -> engine's white 1x1 texture)
	Color color;        // map colour / tint
	float value;        // scalar parameter (meaning depends on the slot)
} MaterialMap;

typedef struct Material {
	MaterialMap maps[MATERIAL_MAP_COUNT]; // indexed by MaterialMapIndex
	float shininess;                      // Phong specular exponent (higher = tighter highlight)
} Material;

// Light kinds:
//   LIGHT_DIRECTIONAL  infinitely far away, parallel rays (the sun). Uses
//                      `direction` only; no position, no distance falloff.
//   LIGHT_POINT        a bulb at `position`, radiating in every direction and
//                      fading with distance (constant/linear/quadratic).
//   LIGHT_SPOT         a point light restricted to a cone aimed along
//                      `direction`; full brightness inside `innerCutoff`,
//                      fading to nothing by `outerCutoff` (soft edge).
typedef enum {
	LIGHT_DIRECTIONAL = 0,
	LIGHT_POINT,
	LIGHT_SPOT
} LightType;

typedef struct Light {
	LightType type;
	bool enabled;      // skipped by the shader when false

	Vector3 position;  // world-space position   (POINT, SPOT)
	Vector3 direction; // direction the light points (DIRECTIONAL, SPOT)
	Vector3 color;     // linear RGB, roughly 0..1

	float ambient;     // strength of the ambient term  (constant fill)
	float diffuse;     // strength of the diffuse term  (Lambert)
	float specular;    // strength of the specular term (Phong highlight)

	// distance attenuation 1 / (constant + linear*d + quadratic*d^2)  (POINT, SPOT)
	float constant;
	float linear;
	float quadratic;

	// spot cone, stored as cosines (cos(inner) >= cos(outer)).  (SPOT)
	float innerCutoff; // full brightness within this angle
	float outerCutoff; // zero brightness beyond this angle; gap = soft edge
} Light;

// --- Mesh ----------------------------------------------------------------
//
// A retained triangle mesh: an interleaved vertex array, a 32-bit index array,
// and a list of textures. It owns its GPU buffers (its own VAO/VBO/EBO) and is
// drawn with whatever shader is active -- so call `Mge_DrawMesh` inside
// `Mge_BeginMode3D` (and, for lighting, inside `Mge_BeginLighting3D[Ex]`).
//
// Vertex positions are world-space (there is no per-mesh model matrix); bake the
// placement into the vertices, or rebuild the mesh to move it.
typedef struct Vertex {
	Vector3 position;
	Vector3 normal;   // for lighting; zero is fine for unlit draws
	Vector2 texcoord;
} Vertex;

typedef enum {
	MESH_TEXTURE_DIFFUSE = 0, // sampled as the surface colour
	MESH_TEXTURE_SPECULAR     // kept on the mesh; not sampled by the built-in shader yet
} MeshTextureType;

typedef struct MeshTexture {
	Texture2D texture;
	MeshTextureType type;
} MeshTexture;

typedef struct Mesh {
	Vertex* vertices;
	int vertexCount;
	unsigned int* indices; // 3 per triangle
	int indexCount;
	MeshTexture* textures;
	int textureCount;

	unsigned int vao, vbo, ebo; // 0 until Mge_UploadMesh
} Mesh;

// --- Model -------------------------------------------------------------------
//
// A `Model` is what `Mge_LoadModel` produces from a file on disk (glTF, OBJ, FBX
// -- anything the vendored Assimp build imports): a flat list of `Mesh`es with
// their node transforms baked into the vertices and their textures loaded from
// `directory`. The meshes are already uploaded to the GPU.
typedef struct Model {
	Mesh* meshes;
	int meshCount;
	char directory[512];       // folder the model was loaded from
	Vector3 bboxMin, bboxMax;  // axis-aligned bounds over every vertex
} Model;

typedef struct Object {
	ObjectKind kind;
	Vector3 position;   // centre; z is unused for OBJECT_2D
	Vector3 size;       // full extents (2D: {w, h, 0})
	Color color;
	Material material;  // surface response for 3D lit rendering
	int id;
	bool selected;
} Object;

// Core
void Mge_InitWindow(uint32_t width, uint32_t height, const char* title);
bool Mge_WindowShouldClose(void);
void Mge_CloseWindow(void);
double Mge_GetTime(void);
double Mge_GetDeltaTime(void);
int Mge_GetFps(void);
void Mge_SetTargetFPS(int fps);
void Mge_BeginShaderMode(Shader shader);
void Mge_EndShaderMode(void);
Shader Mge_LoadShader(const char* vsFileName, const char* fsFileName);
void Mge_UnloadShader(Shader shader);
Shader Mge_LoadShaderFromMemory(const char *vsCode, const char *fsCode);

// Utils
char* Mge_LoadFileText(const char* file_path);
void Mge_UnLoadFileText(char* fileData);
unsigned char* Mge_LoadFileData(const char *fileName, size_t *dataSize);
void Mge_UnloadFileData(unsigned char *data);
const char* Mge_GetFileExtension(const char *fileName);

// Keyboard
bool IsKeyPressed(int key);
bool IsKeyDown(int key);
bool IsKeyUp(int key);
bool IsKeyReleased(int key);

// Mouse
float GetMouseX(void);
float GetMouseY(void);
Vector2 GetMousePosition(void);
Vector2 GetMouseDelta(void);
void SetMousePosition(int x, int y);
void ShowCursor(void);       // make the cursor visible
void HideCursor(void);       // hide the cursor (still free to move)
void EnableCursor(void);     // show + unlock the cursor
void DisableCursor(void);    // hide + lock the cursor to the window (FPS style)
bool IsCursorHidden(void);   // true while the cursor is hidden or locked
void Mge_ToggleCursor(void); // flip between EnableCursor() and DisableCursor()
bool IsMouseButtonPressed(int button);
bool IsMouseButtonDown(int button);
bool IsMouseButtonReleased(int button);

// Window
int Mge_GetScreenWidth(void);
int Mge_GetScreenHeight(void);

// Texture
Image Mge_LoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize);
Image Mge_LoadImage(const char* fileName);
void Mge_UnloadImage(Image image);
Texture2D Mge_LoadTextureFromImage(Image image);
Texture2D Mge_LoadTexture(const char *fileName);

void Mge_ClearBackground(Color color);
void Mge_BeginDrawing(void);
void Mge_EndDrawing(void);
void Mge_BeginMode3D(Camera3D camera);
void Mge_EndMode3D(void);

// Shapes
void Draw_Line(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
void Draw_LineV(Vector2 startPos, Vector2 endPos, Color color);
void Draw_Rectangle(int posX, int posY, int width, int height, Color color);
void Draw_RectangleV(Vector2 position, Vector2 size, Color color);
void Draw_RectangleRec(Rectangle rec, Color color);
void Draw_RectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);
void Draw_RectangleLines(int posX, int posY, int width, int height, Color color);
void Draw_Triangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
void Draw_TriangleLines(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
void Draw_TriangleFan(Vector2 *points, int pointCount, Color color);
void Draw_TriangleStrip(Vector2 *points, int pointCount, Color color);
void Draw_Arrow(Vector2 start, Vector2 end, float headSize, Color color);   // 2D
void Draw_Arrow3D(Vector3 start, Vector3 end, Color color);                 // call inside Mge_BeginMode3D
void Draw_Cube(Vector3 center, Vector3 size, Color color);
void Draw_CubeWires(Vector3 center, Vector3 size, Color color);

// Camera / projection helpers (3D)
Matrix Mge_GetCameraViewMatrix(Camera3D camera);
Matrix Mge_GetCameraProjectionMatrix(Camera3D camera, float aspect);
Vector2 Mge_GetWorldToScreen(Vector3 position, Camera3D camera);
Vector2 Mge_GetWorldToScreenEx(Vector3 position, Camera3D camera, int screenWidth, int screenHeight);

// Objects
Object Mge_MakeObject2D(float x, float y, float w, float h, Color color);
Object Mge_MakeObject3D(Vector3 position, Vector3 size, Color color);
void   Mge_DrawObject(Object obj);
void   Mge_DrawObjectGizmo(Object obj, float axisLength);   // X/Y (2D) or X/Y/Z (3D) arrows at obj.position

// Mesh -- copies the arrays you pass, then owns them. Upload once, draw many.
Mesh Mge_MakeMesh(const Vertex* vertices, int vertexCount,
    const unsigned int* indices, int indexCount,
    const MeshTexture* textures, int textureCount);
void Mge_UploadMesh(Mesh* mesh);   // create the GPU buffers (no-op if already uploaded)
void Mge_DrawMesh(Mesh mesh);      // inside Mge_BeginMode3D (+ Mge_BeginLighting3D for lighting)
void Mge_UnloadMesh(Mesh* mesh);   // free GPU + CPU data and zero the struct

// Model -- load a mesh file via Assimp; meshes come back GPU-ready.
Model Mge_LoadModel(const char* path);
void  Mge_DrawModel(Model model);   // draws every mesh; inside Mge_BeginMode3D
void  Mge_UnloadModel(Model* model);

// Mouse-driven manipulation. Call once per frame while the cursor is enabled.
// Left-click an object to select it, then drag a gizmo arrow to move along that
// axis, or drag the body to move freely. Returns the selected index, or -1.
int Mge_ManipulateObjects2D(Object* objects, int count, float axisLength);
int Mge_ManipulateObjects3D(Object* objects, int count, Camera3D camera, float axisLength);
void Mge_ClearSelection(Object* objects, int count); // deselect all, cancel any drag
int Mge_GetSelectedObject(void);                     // index of the selected object, or -1

#ifndef MGE_MAX_LIGHTS
	#define MGE_MAX_LIGHTS 8   // shader hard limit; extra lights are ignored
#endif

// Lighting. Call inside Mge_BeginMode3D:
//
//   Light sun   = Mge_MakeDirectionalLight((Vector3){ -1, -2, -1 }, (Vector3){ 1, 1, 1 });
//   Light lamp  = Mge_MakePointLight((Vector3){ 3, 4, 2 }, (Vector3){ 1, .6f, .2f });
//   Light torch = Mge_MakeFlashlight(camera, (Vector3){ 1, 1, 1 });
//   Light lights[] = { sun, lamp, torch };
//
//   Mge_BeginLighting3DEx(lights, 3, camera);      // or Mge_BeginLighting3D(sun, camera) for one
//       Mge_SetMaterial(mat);   Draw_Cube(...);
//   Mge_EndLighting3D();
//
// Mge_DrawObject() already calls Mge_SetMaterial(obj.material) for you, so an
// Object drawn between Begin/End is lit with its own material automatically.
Material Mge_DefaultMaterial(void);                      // white diffuse, no textures, shininess 32
void     Mge_SetMaterialTexture(Material* material, int mapIndex, Texture2D texture); // assign one map's texture

Light Mge_MakeLight(Vector3 position, Vector3 color);            // point light, no distance falloff (legacy default)
Light Mge_MakeDirectionalLight(Vector3 direction, Vector3 color); // the sun: parallel rays, no position
Light Mge_MakePointLight(Vector3 position, Vector3 color);        // a bulb: radiates + fades with distance
Light Mge_MakeSpotLight(Vector3 position, Vector3 direction, Vector3 color,
    float innerAngleDeg, float outerAngleDeg);                    // a cone; inner<outer gives a soft edge
Light Mge_MakeFlashlight(Camera3D camera, Vector3 color);         // a tight spot at the camera, aimed where it looks

void Mge_BeginLighting3D(Light light, Camera3D camera);                       // one light
void Mge_BeginLighting3DEx(const Light* lights, int count, Camera3D camera);  // up to MGE_MAX_LIGHTS
void Mge_SetMaterial(Material material);                 // no-op unless lighting is active
void Mge_EndLighting3D(void);
