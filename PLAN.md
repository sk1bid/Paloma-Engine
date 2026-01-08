# Paloma Engine — Полный План Реализации

Metal 4 движок с EnTT ECS, Frame Graph рендерингом, MetalFX и ML интеграцией.

---

## 📚 Соглашение об Обучении

> **Роль наставника:** Я — не простой исполнитель, а учитель и наставник. Моя задача — не просто выдать готовый код, а научить тебя понимать каждую строку, каждую концепцию.

### Методология обучения

#### 1. Теория первична
Перед написанием любого кода я объясняю:
- **Зачем** это нужно (мотивация)
- **Как** это работает (архитектура)
- **Почему** именно так (сравнение альтернатив)

#### 2. Код малыми порциями
- Каждый блок кода — **не более 20-30 строк**
- После каждого блока — объяснение **каждой строки**
- Следующий блок только после полного понимания предыдущего

#### 3. Логический порядок
1. Сначала **концептуальная модель** (что хотим получить)
2. Затем **сигнатуры** (какие функции/классы нужны)
3. После этого **реализация** (построчно с комментариями)
4. В конце **верификация** (проверка работоспособности)

#### 4. Не бежать впереди паровоза
- Один файл за раз
- Компиляция и проверка после каждого этапа
- Возврат к теории при любых вопросах

### Формат урока

```
📖 ТЕОРИЯ: [Название концепции]
   └── Объяснение на 3-5 абзацев

🔧 ЗАДАЧА: [Что будем делать]
   └── Описание цели этого блока

💻 КОД: [Файл.hpp/cpp]
   └── Маленький блок (10-20 строк)
   └── Комментарии к каждой строке

✅ ПРОВЕРКА: [Что должно произойти]
   └── Как убедиться что работает
```

### Правила взаимодействия

| Правило | Описание |
|---------|----------|
| **Задавай вопросы** | Если что-то непонятно — остановись и спроси |
| **Не копируй слепо** | Пиши код самостоятельно, понимая каждую строку |
| **Экспериментируй** | Меняй параметры, смотри что происходит |
| **Ошибки — это хорошо** | Каждая ошибка — возможность понять глубже |

---

## Используемые Технологии

### Apple Frameworks
| Фреймворк | Назначение |
|-----------|------------|
| **Metal** | GPU API (через metal-cpp) |
| **MetalKit** | MTKView, загрузка текстур/мешей |
| **ModelIO** | Загрузка 3D моделей |
| **MetalFX** | Апскейлинг, интерполяция кадров |
| **AppKit** | Окна, события |
| **simd** | Векторная математика |

### Внешние Библиотеки
| Библиотека | Версия | Назначение |
|------------|--------|------------|
| **metal-cpp** | latest | C++ биндинги для Metal 4 |
| **EnTT** | 3.13+ | Entity-Component-System |

---

## Структура Проекта

```
PalomaEngine/
├── Source/
│   ├── Core/           # Ядро Metal 4
│   ├── Renderer/       # Frame Graph, пассы
│   ├── Scene/          # EnTT ECS, камера
│   ├── Assets/         # Меши, материалы
│   ├── Input/          # Ввод
│   ├── Platform/       # Obj-C++ мосты
│   └── Shaders/        # Metal шейдеры
├── Demos/              # Демо-сцены
├── App/                # Точка входа
├── Libs/
│   ├── metal-cpp/
│   └── entt/
└── Resources/          # Ассеты
```

---

## Неделя 1: Ядро Metal 4

### Цель: Куб на экране с Metal 4 API

---

#### [НОВЫЙ] [Types.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Core/Types.hpp)
Базовые типы и SIMD алиасы.
```cpp
namespace Paloma {
    using float2 = simd::float2;
    using float3 = simd::float3;
    using float4 = simd::float4;
    using float4x4 = simd::float4x4;
    using quatf = simd::quatf;
    
    constexpr uint32_t MaxFramesInFlight = 3;
}
```

---

#### [НОВЫЙ] [Context.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Core/Context.hpp)
Центральный Metal 4 контекст.
```cpp
class Context {
    MTL::Device*              _device;
    MTL4::CommandQueue*       _commandQueue;
    MTL4::CommandAllocator*   _allocators[3];
    MTL4::CommandBuffer*      _commandBuffer;
    MTL4::Compiler*           _compiler;
    MTL4::ArgumentTable*      _argumentTable;
    MTL::ResidencySet*        _residencySet;
    MTL::SharedEvent*         _frameEvent;
    
public:
    void init();
    void shutdown();
    void beginFrame();
    void endFrame(CA::MetalDrawable* drawable);
    
    // Геттеры
    MTL::Device* device();
    MTL4::CommandBuffer* commandBuffer();
    MTL4::Compiler* compiler();
    MTL4::ArgumentTable* argumentTable();
};
```

---

#### [НОВЫЙ] [Context.cpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Core/Context.cpp)
**Реализация:**
1. `init()` — создание device, queue, allocators, compiler, residency set
2. `beginFrame()` — ожидание кадра N-3, сброс аллокатора, начало command buffer
3. `endFrame()` — завершение buffer, commit, signal event, present

---

#### [НОВЫЙ] [Bridge.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Platform/Bridge.hpp)
C++ интерфейс к Objective-C API.
```cpp
namespace Bridge {
    MTL4::RenderPassDescriptor* currentRenderPassDescriptor(void* view);
    CA::MetalDrawable* currentDrawable(void* view);
    MTL::ResidencySet* layerResidencySet(void* view);
    void waitForEvent(MTL::SharedEvent* event, uint64_t value, uint64_t timeout);
    CGSize viewSize(void* view);
}
```

---

#### [НОВЫЙ] [Bridge.mm](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Platform/Bridge.mm)
Obj-C++ реализация с `(__bridge ...)` кастами.

---

#### [НОВЫЙ] [main.mm](file:///Users/sk1bid/Desktop/Paloma%20Engine/App/main.mm)
Точка входа macOS приложения.

#### [НОВЫЙ] [AppDelegate.mm](file:///Users/sk1bid/Desktop/Paloma%20Engine/App/AppDelegate.mm)
Делегат приложения, создание окна.

#### [НОВЫЙ] [MTKViewDelegate.mm](file:///Users/sk1bid/Desktop/Paloma%20Engine/App/MTKViewDelegate.mm)
Callback `drawInMTKView:` — вызывает Engine::render().

---

### Файлы Недели 1
| Файл | Строк | Назначение |
|------|-------|------------|
| `Types.hpp` | ~30 | Типы |
| `Context.hpp/cpp` | ~200 | Metal 4 ядро |
| `Bridge.hpp/mm` | ~80 | Obj-C++ мост |
| `main.mm` | ~50 | Точка входа |
| `AppDelegate.mm` | ~40 | Делегат |
| `MTKViewDelegate.mm` | ~50 | Render loop |

---

## Неделя 2: Ресурсы и Биндинг

### Цель: Несколько объектов с текстурами

---

#### [НОВЫЙ] [Mesh.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Assets/Mesh.hpp)
```cpp
struct Submesh {
    uint32_t indexStart;
    uint32_t indexCount;
    MTL::IndexType indexType;
};

class Mesh {
    MTL::Buffer* _vertexBuffer;
    MTL::Buffer* _indexBuffer;
    std::vector<Submesh> _submeshes;
public:
    MTL::GPUAddress vertexAddress() const;
    MTL::GPUAddress indexAddress() const;
};
```

---

#### [НОВЫЙ] [MeshLoader.mm](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Platform/MeshLoader.mm)
Загрузка через `MDLMesh` + `MTKMesh`.

---

#### [НОВЫЙ] [TextureLoader.mm](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Platform/TextureLoader.mm)
Загрузка через `MTKTextureLoader`.

---

#### [НОВЫЙ] [AssetManager.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Assets/AssetManager.hpp)
```cpp
class AssetManager {
    std::unordered_map<std::string, Mesh*> _meshes;
    std::unordered_map<std::string, MTL::Texture*> _textures;
public:
    Mesh* getMesh(const char* name);
    MTL::Texture* getTexture(const char* name);
    void registerWithResidencySet(MTL::ResidencySet* set);
};
```

---

### Файлы Недели 2
| Файл | Строк | Назначение |
|------|-------|------------|
| `Mesh.hpp` | ~50 | Данные меша |
| `MeshLoader.mm` | ~100 | ModelIO мост |
| `TextureLoader.mm` | ~60 | Загрузка текстур |
| `AssetManager.hpp/cpp` | ~120 | Кеш ресурсов |

---

## Неделя 3: Пайплайны и Материалы

### Цель: PBR материалы с разными свойствами

---

#### [НОВЫЙ] [Material.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Assets/Material.hpp)
```cpp
struct Material {
    std::string name;
    MTL::RenderPipelineState* pipeline;
    
    // Текстуры (ResourceID для ArgumentTable)
    MTL::ResourceID albedoMap;
    MTL::ResourceID normalMap;
    MTL::ResourceID metallicRoughnessMap;
    
    // Параметры по умолчанию
    float4 albedoColor = {1,1,1,1};
    float metallic = 0.0f;
    float roughness = 0.5f;
};
```

---

#### [НОВЫЙ] [PipelineCache.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Renderer/PipelineCache.hpp)
```cpp
class PipelineCache {
    std::unordered_map<size_t, MTL::RenderPipelineState*> _cache;
public:
    MTL::RenderPipelineState* getPipeline(const PipelineDesc& desc);
    MTL::RenderPipelineState* specialize(MTL::RenderPipelineState* base, 
                                         const BlendDesc& blend);
};
```

---

#### [НОВЫЙ] [ShaderTypes.h](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Shaders/ShaderTypes.h)
Общие типы для C++ и Metal.
```cpp
struct FrameUniforms {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float3   cameraPosition;
    float    time;
};

struct InstanceData {
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

enum BufferIndex {
    BufferIndexFrameUniforms = 0,
    BufferIndexInstanceData  = 1,
    BufferIndexVertices      = 2,
};
```

---

#### [НОВЫЙ] [PBR.metal](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Shaders/PBR.metal)
PBR шейдеры с поддержкой ArgumentTable.

---

### Файлы Недели 3
| Файл | Строк | Назначение |
|------|-------|------------|
| `Material.hpp` | ~50 | Материал |
| `PipelineCache.hpp/cpp` | ~150 | Кеш пайплайнов |
| `ShaderTypes.h` | ~60 | Общие типы |
| `PBR.metal` | ~200 | Шейдеры |

---

## Неделя 4: Frame Graph

### Цель: Многопроходный рендеринг с автоматическими барьерами

---

#### [НОВЫЙ] [FrameGraph.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Renderer/FrameGraph.hpp)
```cpp
struct ResourceHandle { uint32_t id; };

class FrameGraph {
    struct PassNode {
        std::string name;
        std::vector<ResourceHandle> reads;
        std::vector<ResourceHandle> writes;
        std::function<void(PassContext&)> execute;
    };
    
    std::vector<PassNode> _passes;
    
public:
    ResourceHandle createTexture(const char* name, TextureDesc desc);
    ResourceHandle importBackbuffer(MTL::Texture* tex);
    
    template<typename T>
    void addPass(const char* name, 
                 std::function<void(T&, Builder&)> setup,
                 std::function<void(const T&, PassContext&)> exec);
    
    void compile();   // Построить граф зависимостей
    void execute(Context& ctx);  // Выполнить пассы
};
```

---

#### [НОВЫЙ] [BuiltinPasses.cpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Renderer/Passes/BuiltinPasses.cpp)
```cpp
void addOpaquePass(FrameGraph& fg, entt::registry& reg, Camera& cam);
void addShadowPass(FrameGraph& fg, entt::registry& reg, Light& light);
void addBloomPass(FrameGraph& fg, ResourceHandle hdr);
void addToneMappingPass(FrameGraph& fg, ResourceHandle hdr, ResourceHandle out);
```

---

### Файлы Недели 4
| Файл | Строк | Назначение |
|------|-------|------------|
| `FrameGraph.hpp/cpp` | ~300 | Frame Graph |
| `PassContext.hpp` | ~50 | Контекст пасса |
| `BuiltinPasses.cpp` | ~200 | Стандартные пассы |

---

## Неделя 5: EnTT ECS

### Цель: Иерархия сущностей с EnTT

---

#### [НОВЫЙ] [Components.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Scene/Components.hpp)
```cpp
// Компоненты для EnTT registry

struct TransformComponent {
    float3 position = {0,0,0};
    quatf rotation = simd_quaternion(0,0,0,1);
    float3 scale = {1,1,1};
    float4x4 worldMatrix;
    entt::entity parent = entt::null;
};

struct MeshComponent {
    Mesh* mesh;
    uint32_t submeshIndex = 0;
};

struct MaterialComponent {
    Material* material;
};

struct LightComponent {
    enum Type { Directional, Point, Spot } type;
    float3 color = {1,1,1};
    float intensity = 1.0f;
    float range = 10.0f;
};

struct CameraComponent {
    float fov = 60.0f;
    float near = 0.1f;
    float far = 1000.0f;
};

struct ChildrenComponent {
    std::vector<entt::entity> children;
};
```

---

#### [НОВЫЙ] [Scene.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Scene/Scene.hpp)
```cpp
class Scene {
    entt::registry _registry;
    entt::entity _activeCamera = entt::null;
    
public:
    entt::registry& registry() { return _registry; }
    
    entt::entity createEntity();
    void destroyEntity(entt::entity e);
    void setParent(entt::entity child, entt::entity parent);
    
    entt::entity activeCamera();
    void setActiveCamera(entt::entity cam);
};
```

---

#### [НОВЫЙ] [Systems.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Scene/Systems.hpp)
```cpp
class TransformSystem {
public:
    void update(entt::registry& reg);
private:
    void updateRecursive(entt::registry& reg, entt::entity e, float4x4 parentWorld);
};

class RenderSystem {
public:
    struct DrawCall {
        Mesh* mesh;
        Material* material;
        std::vector<float4x4> instances;
    };
    std::vector<DrawCall> extractDrawCalls(entt::registry& reg, Camera& cam);
};
```

---

### Файлы Недели 5
| Файл | Строк | Назначение |
|------|-------|------------|
| `Components.hpp` | ~100 | Компоненты |
| `Scene.hpp/cpp` | ~150 | EnTT враппер |
| `Systems.hpp/cpp` | ~150 | Системы |

---

## Неделя 6: Камера и Ввод

### Цель: WASD + мышь управление

---

#### [НОВЫЙ] [Camera.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Scene/Camera.hpp)
```cpp
class Camera {
public:
    float3 position = {0, 0, 5};
    float3 target = {0, 0, 0};
    float3 up = {0, 1, 0};
    
    float fov = 60.0f;
    float aspect = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 1000.0f;
    
    float4x4 viewMatrix() const;
    float4x4 projectionMatrix() const;
    float3 forward() const;
    float3 right() const;
};
```

---

#### [НОВЫЙ] [Input.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Input/Input.hpp)
```cpp
class Input {
    bool _keys[256] = {};
    bool _keysPressed[256] = {};
    float2 _mousePosition = {0, 0};
    float2 _mouseDelta = {0, 0};
    bool _mouseButtons[3] = {};
    
public:
    bool isKeyDown(uint8_t key) const;
    bool isKeyPressed(uint8_t key) const;
    float2 mouseDelta() const;
    bool isMouseDown(int button) const;
};
```

---

#### [НОВЫЙ] [CameraControllers.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Scene/Controllers/CameraControllers.hpp)
```cpp
// Орбитальная камера (клик-перетаскивание)
class OrbitController {
    float _distance = 5.0f;
    float _yaw = 0.0f, _pitch = 0.0f;
public:
    void update(const Input& input, Camera& cam);
};

// FPS стиль (WASD + мышь)
class FlyController {
    float _yaw = 0.0f, _pitch = 0.0f;
    float _moveSpeed = 5.0f;
public:
    void update(const Input& input, Camera& cam, float dt);
};

// Сплайновая камера (для синематиков)
class PathController {
    struct Key { float time; float3 position; float3 target; };
    std::vector<Key> _keys;
    float _time = 0.0f;
public:
    void addKey(float t, float3 pos, float3 target);
    void update(Camera& cam, float dt);
    bool isFinished() const;
};
```

---

### Файлы Недели 6
| Файл | Строк | Назначение |
|------|-------|------------|
| `Camera.hpp/cpp` | ~100 | Камера |
| `Input.hpp/cpp` | ~100 | Ввод |
| `CameraControllers.hpp/cpp` | ~200 | Контроллеры |

---

## Неделя 7: MetalFX и ML

### Цель: Апскейлинг, интерполяция, нейросети

---

#### [НОВЫЙ] [MetalFXManager.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Renderer/MetalFXManager.hpp)
```cpp
class MetalFXManager {
    MTLFXTemporalScaler* _upscaler;
    MTLFXFrameInterpolator* _interpolator;
public:
    void init(MTL::Device* device, uint32_t renderW, uint32_t renderH,
              uint32_t outputW, uint32_t outputH);
    
    void upscale(MTL::Texture* color, MTL::Texture* depth, 
                 MTL::Texture* motion, MTL::Texture* output);
    
    void interpolate(MTL::Texture* prev, MTL::Texture* curr, MTL::Texture* out);
};
```

---

#### [НОВЫЙ] [MLPassManager.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Source/Renderer/MLPassManager.hpp)
```cpp
class MLPassManager {
    MTL4::MachineLearningPipelineState* _denoisePipeline;
public:
    void loadModel(MTL4::Compiler* compiler, const char* modelPath);
    void denoise(MTL4::CommandBuffer* cmd, MTL::Texture* noisy, MTL::Texture* out);
};
```

---

### Файлы Недели 7
| Файл | Строк | Назначение |
|------|-------|------------|
| `MetalFXManager.hpp/cpp` | ~150 | Апскейлинг |
| `MLPassManager.hpp/cpp` | ~100 | Нейросети |

---

## Неделя 8: Демо-сцены

### Цель: Три полноценные демки

---

#### [НОВЫЙ] [CornellBoxScene.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Demos/CornellBoxScene.hpp)
**Cornell Box (PBR Edition)**
- Комната с цветными стенами (красная/зелёная)
- Два бокса
- Area light сверху
- PBR материалы
- **Тестирует:** Renderer, Materials, Lighting

---

#### [НОВЫЙ] [InstancedForestScene.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Demos/InstancedForestScene.hpp)
**Instanced Forest (10k деревьев)**
- 10,000 инстансов деревьев
- GPU инстансинг
- Compute шейдер для анимации ветра
- Fly камера
- **Тестирует:** ECS производительность, Compute

---

#### [НОВЫЙ] [NeonCityScene.hpp](file:///Users/sk1bid/Desktop/Paloma%20Engine/Demos/NeonCityScene.hpp)
**Neon Fly-through**
- Город с неоновыми огнями
- Path камера (пролёт)
- HDR + Bloom
- MetalFX апскейлинг (опционально)
- **Тестирует:** Frame Graph, Post-FX, PathController

---

### Файлы Недели 8
| Файл | Строк | Назначение |
|------|-------|------------|
| `CornellBoxScene.hpp/cpp` | ~150 | Демо PBR |
| `InstancedForestScene.hpp/cpp` | ~200 | Демо производительности |
| `NeonCityScene.hpp/cpp` | ~250 | Демо FX |
| `DemoSelector.hpp/cpp` | ~50 | Переключатель сцен |

---

## Верификация

### Команды Сборки
```bash
# Сборка
xcodebuild -project "PalomaEngine.xcodeproj" -scheme "PalomaEngine" build

# Запуск демок
./build/PalomaEngine --demo cornell
./build/PalomaEngine --demo forest
./build/PalomaEngine --demo neon
```

### Целевые Показатели
| Демка | FPS | GPU Time |
|-------|-----|----------|
| Cornell Box | 60+ | < 4ms |
| Forest (10k) | 60+ | < 8ms |
| Neon City | 60+ | < 12ms |

---

## Полный Список Файлов

| Неделя | Файлы |
|--------|-------|
| **1** | `Types.hpp`, `Context.hpp/cpp`, `Bridge.hpp/mm`, `main.mm`, `AppDelegate.mm`, `MTKViewDelegate.mm` |
| **2** | `Mesh.hpp`, `MeshLoader.mm`, `TextureLoader.mm`, `AssetManager.hpp/cpp` |
| **3** | `Material.hpp`, `PipelineCache.hpp/cpp`, `ShaderTypes.h`, `PBR.metal` |
| **4** | `FrameGraph.hpp/cpp`, `PassContext.hpp`, `BuiltinPasses.cpp` |
| **5** | `Components.hpp`, `Scene.hpp/cpp`, `Systems.hpp/cpp` |
| **6** | `Camera.hpp/cpp`, `Input.hpp/cpp`, `CameraControllers.hpp/cpp` |
| **7** | `MetalFXManager.hpp/cpp`, `MLPassManager.hpp/cpp` |
| **8** | `CornellBoxScene.hpp/cpp`, `InstancedForestScene.hpp/cpp`, `NeonCityScene.hpp/cpp`, `DemoSelector.hpp/cpp` |

**Всего:** ~45 файлов, ~3500 строк кода
