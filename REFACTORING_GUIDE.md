# Astral Engine Yeniden Düzenleme Kılavuzu

## 1. Giriş

Bu belge, Astral Engine projesini daha modüler, modern ve sürdürülebilir bir yapıya kavuşturmak için bir yol haritası sunmaktadır. Mevcut proje yapısı, CMake kullanımı ve temel modülerlik gibi iyi uygulamaları barındırsa da, daha karmaşık özellikler eklendikçe ve proje büyüdükçe karşılaşılabilecek potansiyel zorlukları ele almayı hedefliyoruz.

Modern bir C++ mimarisine geçişin temel faydaları şunlardır:

*   **Geliştirilmiş Sürdürülebilirlik:** Modüller arasındaki bağımlılıkları netleştirerek, bir modülde yapılan bir değişikliğin diğerlerini beklenmedik şekilde etkilemesini önleriz. Bu, kodun anlaşılmasını, bakımını ve güncellenmesini kolaylaştırır.
*   **Artan Ölçeklenebilirlik:** İyi tanımlanmış arayüzlere sahip bağımsız modüller, yeni özelliklerin veya teknolojilerin (örneğin, farklı bir render backend'i) projeye daha kolay entegre edilmesini sağlar.
*   **İyileştirilmiş Test Edilebilirlik:** Modüllerin birbirinden izole edilmesi, birim testlerinin ve entegrasyon testlerinin yazılmasını basitleştirir. Bu da daha güvenilir ve hatasız bir kod tabanına yol açar.
*   **Daha Hızlı Derleme Süreleri:** Bağımlılıkların optimize edilmesi, yalnızca değiştirilen modüllerin ve onlara bağımlı olanların yeniden derlenmesini sağlayarak geliştirme döngüsünü hızlandırabilir.

Bu kılavuz, mevcut mimariyi analiz edecek, modern bir yapı için önerilerde bulunacak ve bu değişimi gerçekleştirmek için adım adım bir plan sunacaktır.

## 2. Mevcut Mimari Analizi

Mevcut mimari, C++ projeleri için sağlam bir temel oluşturan birkaç modern tekniği zaten kullanmaktadır. Ancak, projenin uzun vadeli sağlığı için iyileştirilebilecek alanlar da bulunmaktadır.

### Güçlü Yönler

*   **CMake Tabanlı Yapı Sistemi:** Proje, platformlar arası derlemeyi kolaylaştıran ve C++ ekosisteminde standart olan CMake'i kullanmaktadır.
*   **Modüler Kaynak Kodu:** Kaynak kodu, `Core`, `Renderer`, `ECS`, `UI` gibi işlevsel modüllere ayrılmıştır. Bu, kodun mantıksal olarak düzenlenmesine yardımcı olur.
*   **Harici Bağımlılık Yönetimi:** `FetchContent` gibi CMake özellikleri, bazı üçüncü taraf kütüphanelerin (örneğin, `fmt`, `imgui`) projeye dahil edilmesini otomatikleştirir.
*   **Otomatik Shader Derlemesi:** Shader'ların otomatik olarak derlenip derleme dizinine kopyalanması, geliştirme sürecini basitleştiren güzel bir özelliktir.

### Zayıf Yönler ve İyileştirme Alanları

*   **Belirsiz Bağımlılık Grafiği:** `src/CMakeLists.txt` dosyasında, tüm alt modüller `AstralEngine` adlı tek bir `INTERFACE` kütüphanesinde birleştirilmiştir. Bu, modüller arasındaki gerçek bağımlılıkları gizler. Örneğin, `Renderer`'ın `ECS`'ye olan bağımlılığı sadece `Renderer/CMakeLists.txt` dosyasına bakıldığında anlaşılabilir. Bu durum, "her şey her şeye bağlı" gibi bir yapıya yol açabilir ve modülerliği zayıflatır.
*   **Sıkı Bağlantı (Tight Coupling):** `Renderer` modülünün `AstralCore` ve `AstralECS`'ye doğrudan bağımlı olması, bu modüller arasında sıkı bir bağlantı olduğunu gösterir. İdeal olarak, `Renderer` yalnızca kendi işlevselliği için gerekli olan minimum veri yapılarına ve arayüzlere bağımlı olmalıdır.
*   **Global Durum ve Singleton'lar (Tahmin):** `Core` dizininde `AssetManager`, `Logger` gibi isimler, genellikle singleton veya global olarak erişilen sistemleri ima eder. Bu tür tasarımlar, test edilebilirliği zorlaştırır ve kodun farklı bağlamlarda yeniden kullanılmasını engeller.
*   **CMake Kod Tekrarı:** Kök `CMakeLists.txt` dosyasında, `AstralCreativeSuite` ve `Astral3DEditor` hedefleri için `target_link_libraries` ve `target_include_directories` komutları neredeyse aynıdır. Bu, kod tekrarına yol açar ve yeni bir uygulama eklendiğinde bakım maliyetini artırır.
*   **Test Altyapısının Eksikliği:** `test_renderer.cpp` dışında projede özel bir test altyapısı (örneğin, Google Test veya Catch2) bulunmamaktadır. Bu, kod kalitesini ve güvenilirliğini sağlamayı zorlaştırır.
*   **Render API'sinin Sıkı Bağlılığı:** `Renderer` modülü, doğrudan Vulkan API'sine karşı yazılmıştır. Bu, gelecekte DirectX veya Metal gibi başka render API'lerini desteklemeyi zorlaştırır.

## 3. Önerilen Modern Mimari

Bu bölümde, Astral Engine'i daha sağlam, esnek ve bakımı kolay bir hale getirecek yeni mimari önerileri sunulmaktadır.

### 3.1. Üst Düzey Dizin Yapısı

Projenin kök dizin yapısını, sorumlulukları daha net ayıracak şekilde yeniden düzenlemeyi öneriyoruz:

*   **`root/`**
    *   **`apps/`**: Son kullanıcı uygulamalarını (editörler, oyunlar) içerir.
        *   `AstralEditor/`
        *   `Game/` (Örnek)
    *   **`engine/`**: Tek başına bir kütüphane olarak derlenebilecek olan motorun kaynak kodunu içerir.
        *   `core/`
        *   `ecs/`
        *   `renderer/`
        *   `...`
    *   **`external/`**: `FetchContent` veya bir paket yöneticisi (Conan/vcpkg) tarafından yönetilenler dışındaki üçüncü taraf kütüphaneleri (örneğin, `glm`, `stb`) içerir.
    *   **`scripts/`**: Yardımcı betikleri (örneğin, proje oluşturma, derleme) içerir.
    *   **`tests/`**: Motor ve uygulamalar için test kodlarını içerir.
    *   `CMakeLists.txt` (Kök)

Bu yapı, motorun kendisi ile onu kullanan uygulamalar arasında net bir ayrım sağlar.

### 3.2. Modüler Motor Tasarımı ve Bağımlılık Grafiği

Modüller arasındaki bağımlılıkları netleştirmek, en önemli yeniden düzenleme hedeflerinden biridir.

**Önerilen Bağımlılık Grafiği:**

*   **`core`**: En temel modül. Hiçbir diğer motor modülüne bağımlı olmamalıdır. Yalnızca C++ standart kütüphanesi ve `external` kütüphanelere (örn. `fmt`) bağımlı olabilir.
*   **`platform`**: `core`'a bağımlıdır. Pencere yönetimi, girdi gibi platforma özgü işlevleri soyutlar.
*   **`ecs`**: `core`'a bağımlıdır. Varlık ve bileşen yönetimi sağlar.
*   **`asset`**: `core`'a bağımlıdır. Varlıkların yüklenmesi ve yönetilmesinden sorumludur.
*   **`renderer`**: `core`, `platform` ve `asset`'e bağımlıdır. `ecs`'ye *doğrudan* bağımlı olmamalıdır. Render sistemi, render edilecek nesneler hakkında bilgiyi `ecs`'den doğrudan almak yerine, render komutları veya arayüzler aracılığıyla dolaylı olarak almalıdır. Bu, render motorunun başka projelerde `ecs` olmadan da kullanılabilmesini sağlar.
*   **`ui`**: `core`, `platform` ve `renderer`'a bağımlıdır.

Bu bağımlılıklar, CMake'de `target_link_libraries` komutuyla `PRIVATE` olarak belirtilerek zorunlu hale getirilmelidir.

### 3.3. Render API Soyutlaması

`Renderer` modülünü Vulkan'a sıkı sıkıya bağlamak yerine, bir soyutlama katmanı oluşturulmalıdır.

*   **`Renderer` Arayüzü:** `Texture`, `Buffer`, `Shader`, `Pipeline` gibi temel render kavramlarını tanımlayan soyut sınıflar veya arayüzler oluşturun.
*   **`VulkanRenderer` Uygulaması:** Bu arayüzleri Vulkan API'sini kullanarak somut bir şekilde uygulayın.
*   **Gelecek için Esneklik:** Bu yapı, gelecekte `DirectX12Renderer` veya `MetalRenderer` gibi yeni arka uçların eklenmesini çok daha kolay hale getirir.

### 3.4. Bağımlılık Enjeksiyonu ve Bağlam (Context) Nesnesi

Global singleton'lar yerine, sistemlerin birbirine olan bağımlılıklarını açıkça yönetmek için bir "Bağlam" (Context) nesnesi kullanın.

```cpp
// Örnek Context yapısı
struct EngineContext {
    Logger* logger;
    AssetManager* assetManager;
    EventManager* eventManager;
};

// Bir sistem bu context'i kurucu metotta alabilir
class SomeSystem {
public:
    SomeSystem(EngineContext* context) : m_Context(context) {}
    void DoSomething() {
        m_Context->logger->Info("Bir şeyler yapılıyor...");
    }
private:
    EngineContext* m_Context;
};
```

Bu yaklaşım, sistemleri daha test edilebilir ve yeniden kullanılabilir hale getirir.

### 3.5. CMake İyileştirmeleri

*   **Paket Yöneticisi:** `SDL3`, `Vulkan`, `fmt` gibi harici bağımlılıkları yönetmek için [Conan](https://conan.io/) veya [vcpkg](https://vcpkg.io/) gibi bir C++ paket yöneticisi entegre edin. Bu, projenin farklı ortamlarda kurulmasını ve derlenmesini büyük ölçüde basitleştirir.
*   **CMake Fonksiyonları:** Yeni bir motor modülü veya uygulama oluşturma sürecini basitleştirmek için CMake fonksiyonları yazın.

    ```cmake
    # Örnek bir fonksiyon
    function(add_engine_module module_name)
        add_library(${module_name} ...)
        target_include_directories(${module_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
        # ... diğer ortak ayarlar
    endfunction()
    ```
*   **Açık ve Özel Bağlantı:** `target_link_libraries` kullanırken, bağımlılıkları `PUBLIC` yerine `PRIVATE` olarak belirtmeye özen gösterin. Bir bağımlılık, yalnızca o modülün başlık dosyalarında (`.h`) kullanılıyorsa `PUBLIC` olmalıdır. Bu, bağımlılıkların sızmasını önler ve derleme sürelerini iyileştirir.

## 4. Yeniden Düzenleme Adımları (Yol Haritası)

Bu bölümde, önerilen mimariye geçiş için izlenmesi gereken adımlar anlatılmaktadır. Bu adımların sırayla ve dikkatli bir şekilde uygulanması, sürecin sorunsuz ilerlemesini sağlayacaktır.

### Adım 1: Proje Yeniden Yapılandırması ve Test Altyapısı

1.  **Yeni Dizinleri Oluşturun:** `apps`, `engine`, `external`, `scripts`, `tests` dizinlerini oluşturun.
2.  **Mevcut Kaynakları Taşıyın:**
    *   `src/` altındaki tüm modül dizinlerini (`Core`, `ECS`, vb.) `engine/` altına taşıyın.
    *   `editor_main.cpp` ve `3d_editor_main.cpp` dosyalarını `apps/AstralEditor/` gibi yeni bir dizine taşıyın.
    *   `external/` dizinindeki kütüphaneleri yeni `external/` dizinine taşıyın.
3.  **Test Altyapısını Kurun:**
    *   `tests/` dizini oluşturun.
    *   Google Test gibi bir test çerçevesini `FetchContent` veya seçtiğiniz paket yöneticisi ile projeye ekleyin.
    *   Basit bir test (örneğin, `1+1=2`) yazarak altyapının çalıştığını doğrulayın.

### Adım 2: CMake Dosyalarını Yeniden Düzenleme

1.  **Kök `CMakeLists.txt`'yi Basitleştirin:**
    *   Yalnızca `add_subdirectory(engine)`, `add_subdirectory(apps)` ve `add_subdirectory(tests)` gibi üst düzey komutları içermelidir.
    *   Paket yöneticisi entegrasyonunu (eğer kullanılıyorsa) burada yapın.
2.  **`engine/CMakeLists.txt` Oluşturun:**
    *   Bu dosya, `engine` altındaki tüm modülleri `add_subdirectory` ile eklemelidir.
    *   Motor modülleri için ortak ayarları (örneğin, C++ standardı) burada yapın.
3.  **Modül `CMakeLists.txt`'lerini Güncelleyin:**
    *   Her modülün `CMakeLists.txt` dosyasını, **yalnızca doğrudan bağımlı olduğu** diğer modüllere `target_link_libraries` ile `PRIVATE` olarak bağlanacak şekilde güncelleyin.
    *   Örneğin, `engine/renderer/CMakeLists.txt` içinde: `target_link_libraries(AstralRenderer PRIVATE AstralCore AstralPlatform ...)`
4.  **Uygulama `CMakeLists.txt`'lerini Oluşturun:**
    *   `apps/AstralEditor/CMakeLists.txt` dosyasını oluşturun.
    *   Bu dosya, editörün çalıştırılabilir hedefini oluşturmalı ve `AstralEngine`'in ilgili modüllerine bağlanmalıdır.

### Adım 3: Render API'sini Soyutlama

1.  **Arayüzleri Tanımlayın:** `engine/renderer/include/` altında `rhi` (Rendering Hardware Interface) veya `gfx` gibi bir dizin oluşturun. `Texture.h`, `Buffer.h`, `CommandBuffer.h` gibi soyut arayüzleri burada tanımlayın.
2.  **Vulkan Arka Ucunu Oluşturun:** `engine/renderer/src/vulkan/` altında, bu arayüzleri implemente eden Vulkan'a özgü sınıfları oluşturun.
3.  **Renderer'ı Güncelleyin:** `Renderer` sınıfının, somut Vulkan nesneleri yerine bu soyut arayüzlerle çalışmasını sağlayın.

### Adım 4: Çekirdek Sistemleri ve Bağımlılık Enjeksiyonu

1.  **`EngineContext` Yapısını Oluşturun:** `engine/core/include/` altında `EngineContext.h` dosyasını oluşturun ve `Logger`, `AssetManager` gibi sistemlere işaretçiler içermesini sağlayın.
2.  **Sistemleri Güncelleyin:** `Renderer`, `UI` gibi sistemlerin kurucu metotlarını, `EngineContext`'i parametre olarak alacak şekilde değiştirin.
3.  **Uygulama Başlatmayı Değiştirin:** Uygulamanın `main` fonksiyonunda, tüm sistemleri oluşturun, `EngineContext`'i doldurun ve bu context'i diğer sistemlere geçirin.

### Adım 5: Uygulama Kodunu Uyarlama

1.  **Yeni API'leri Kullanın:** Editör kodunu, soyutlanmış render API'si ve `EngineContext` aracılığıyla erişilen sistemler gibi yeni yapıları kullanacak şekilde güncelleyin.
2.  **Derleme ve Test:** Tüm projenin başarıyla derlendiğinden ve testlerin geçtiğinden emin olun.

Bu adımlar tamamlandığında, proje daha modüler, esnek ve gelecekteki geliştirmelere hazır bir yapıya kavuşmuş olacaktır.

## 5. Sonuç

Bu kılavuzda özetlenen yeniden düzenleme süreci, Astral Engine projesini önemli ölçüde iyileştirme potansiyeline sahiptir. Önerilen değişiklikler uygulandığında, proje aşağıdaki kazanımları elde edecektir:

*   **Daha Net ve Anlaşılır Kod Tabanı:** Modüller arasındaki sorumlulukların ve bağımlılıkların net bir şekilde tanımlanması, yeni geliştiricilerin projeye adapte olmasını ve mevcut geliştiricilerin kodu anlamasını kolaylaştıracaktır.
*   **Artan Esneklik:** Render API'sinin soyutlanması gibi değişiklikler, gelecekte yeni teknolojileri (örneğin, DirectX 12) veya platformları (örneğin, macOS) desteklemeyi çok daha basit hale getirecektir.
*   **Gelişmiş Kod Kalitesi ve Güvenilirlik:** Özel bir test altyapısının kurulması ve bağımlılık enjeksiyonu gibi desenlerin benimsenmesi, daha sağlam, test edilebilir ve güvenilir bir yazılım ortaya çıkaracaktır.

Bu yeniden düzenleme, sadece bir teknik borç ödemesi değil, aynı zamanda projenin gelecekteki büyümesi ve başarısı için sağlam bir temel oluşturan bir yatırımdır.

### Sonraki Adımlar

Yeniden düzenleme tamamlandıktan sonra, aşağıdaki adımlar projenin daha da geliştirilmesine yardımcı olabilir:

*   **Sürekli Entegrasyon (CI) Kurulumu:** GitHub Actions gibi bir CI sistemi kurarak, her kod değişikliğinde testlerin otomatik olarak çalıştırılmasını ve projenin her zaman derlenebilir durumda kalmasını sağlayın.
*   **Belgelendirme:** Kod içi belgeleri (örneğin, Doxygen) ve genel mimari belgelerini iyileştirerek proje bilgisini güncel tutun.
*   **Performans Profillemesi:** Düzenli olarak performans profillemesi yaparak ve darboğazları belirleyerek motorun verimliliğini artırın.
