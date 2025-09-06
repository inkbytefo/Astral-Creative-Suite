# Proje Brifingi: Astral Oyun Motoru

**Proje Adı:** Astral Engine  
**Versiyon:** 1.0  
**Tarih:** 6 Eylül 2025  
**Hazırlayan:** inkbytefo  

---

## 1. Özet (Executive Summary)

Bu belge, Astral Engine adlı 3D oyun motorunun sıfırdan geliştirilmesi için bir yol haritası ve ana planı niteliğindedir. Proje, modern C++ ve Vulkan grafik API'si kullanılarak, modüler, performans odaklı ve genişletilebilir bir mimari oluşturmayı hedefler. Bu süreç, hem güçlü bir portfolyo projesi oluşturma hem de modern oyun motoru mimarileri üzerine derinlemesine bilgi edinme amacı taşımaktadır.

---

## 2. Projenin Amacı ve Vizyonu

**Vizyon:** Bağımsız geliştiricilerin ve hobi amaçlı kodlayıcıların, karmaşık alt seviye detaylarla boğuşmadan, yaratıcı fikirlerini hayata geçirebilecekleri, temiz ve esnek bir oyun motoru yaratmak.  

**Temel Amaçlar:**
- **Öğrenme ve Uzmanlaşma:** Modern C++, 3D grafik programlama (Vulkan), ve yazılım mimarisi konularında derinlemesine uzmanlık kazanmak.
- **Performans:** Veri odaklı tasarım (Data-Oriented Design) prensiplerini benimseyerek ve modern GPU özelliklerinden faydalanarak yüksek performanslı bir render altyapısı kurmak.
- **Modülerlik:** Motorun her bir parçasının (render, fizik, ses) birbirinden bağımsız olarak geliştirilebilmesini ve değiştirilebilmesini sağlamak.
- **Portfolyo:** Kariyer hedefleri için sergilenebilecek, kapsamlı ve teknik olarak etkileyici bir proje ortaya çıkarmak.

---

## 3. Hedef Kitle

- **Birincil:** Proje geliştiricisi (kendin) - Öğrenme ve kendini geliştirme aracı olarak.
- **İkincil:** Diğer bağımsız oyun geliştiricileri - Küçük ve orta ölçekli 3D oyunlar yapmak için bir alternatif arayanlar.

---

## 4. Temel Özellikler (Core Features)

### Render Sistemi
- Vulkan tabanlı, modern bir render altyapısı.
- Esnek Materyal ve Shader sistemi (şablon ve örnek tabanlı).
- Basit aydınlatma modeli (en azından Phong veya Blinn-Phong).
- Mesh (3D model) render yeteneği.
- Doku (texture) haritalama.

### Varlık (Asset) Yönetimi
- .obj formatında 3D model yükleme.
- .png, .jpg formatlarında doku yükleme.
- Merkezi bir AssetManager ile varlıkların yönetimi ve önbelleğe alınması.

### Sahne Yönetimi
- Entity-Component-System (ECS) tabanlı sahne mimarisi.
- Temel component'lar: TransformComponent, RenderComponent.
- Basit bir sahne hiyerarşisi.

### Platform ve Çekirdek
- Platformdan bağımsız pencere ve girdi yönetimi (SDL3 ile).
- Yapılandırma dosyası (.json) desteği.
- Detaylı loglama ve hata yönetimi sistemi.

---

## 5. Teknoloji Yığını (Technology Stack)

- **Programlama Dili:** C++17 (veya üstü)  
- **Build Sistemi:** CMake  
- **Grafik API:** Vulkan 1.3  
- **Pencere/Girdi Kütüphanesi:** SDL3  
- **3D Model Yükleme:** tinyobjloader  
- **Görüntü Yükleme:** stb_image  
- **Bellek Yönetimi:** Vulkan Memory Allocator (VMA) - Şiddetle Tavsiye Edilir  
- **Versiyon Kontrol:** Git  

---

## 6. Proje Kapsamı ve Sınırları

Başlangıçta projenin yönetilebilir kalması için aşağıdaki özellikler kapsam dışıdır:

- AAA seviyesi grafik özellikleri (PBR, Global Illumination, Screen Space Reflections vb. - bunlar daha sonra eklenebilir).  
- Gelişmiş fizik simülasyonu (sadece temel çarpışma tespiti düşünülebilir).  
- Animasyon sistemi.  
- Ses motoru.  
- Görsel bir editör (her şey kod tabanlı ilerleyecek).  
- Ağ (networking) özellikleri.  

---

## 7. Geliştirme Yol Haritası (Milestones)

Proje, yönetilebilir aşamalara bölünmüştür. Her aşama, somut bir çıktı ile sonuçlanacaktır.

### Aşama 0: Kurulum ve Temel Altyapı (Süre: ~1 Hafta)
**Hedef:** Proje iskeletini oluşturmak ve temel araçları kurmak.  
**Çıktılar:**
- Git deposu oluşturulması.  
- CMake build sisteminin kurulması.  
- SDL3 ile boş bir pencere açıp kapatan bir uygulama.  
- Logger ve ConfigurationManager sınıflarının oluşturulması.  

### Aşama 1: "Merhaba Üçgen" - Vulkan Entegrasyonu (Süre: ~2-3 Hafta)
**Hedef:** En temel Vulkan render döngüsünü hayata geçirmek.  
**Çıktılar:**
- Vulkan instance, device ve swap chain oluşturulması.  
- Temizleme rengiyle (clear color) ekranı boyama.  
- Basit bir vertex/fragment shader ile ekrana renkli bir üçgen çizdirme.  

### Aşama 2: 3D Dünyası - Mesh ve Bellek Yönetimi (Süre: ~2 Hafta)
**Hedef:** 3D bir nesneyi yükleyip ekranda gösterebilmek.  
**Çıktılar:**
- Vulkan Memory Allocator (VMA) kütüphanesinin entegrasyonu.  
- Vertex ve index buffer'larını VMA ile oluşturan bir RenderResourceManager.  
- tinyobjloader kullanarak bir .obj dosyasını yükleyen AssetManager.  
- Ekranda dönen bir 3D küp veya model.  

### Aşama 3: Görsellik - Materyal ve Dokular (Süre: ~3 Hafta)
**Hedef:** Nesneleri dokularla kaplamak ve materyal sistemini kurmak.  
**Çıktılar:**
- stb_image ile doku yükleme.  
- Shader, Material ve MaterialInstance sınıflarının oluşturulması.  
- Descriptor set'leri dinamik olarak yöneten bir sistem.  
- Ekranda dokulu bir 3D model.  

### Aşama 4: Dinamik Sahne - ECS Mimarisi (Süre: ~2 Hafta)
**Hedef:** Birden fazla nesneyi yönetebilen bir sahne sistemi kurmak.  
**Çıktılar:**
- Temel bir Entity-Component-System (ECS) yapısı.  
- TransformComponent ve RenderComponent.  
- Sahnedeki tüm nesneleri çizen bir RenderSystem.  
- Farklı pozisyonlarda duran birden fazla 3D modelin render edildiği bir demo sahne.  

---

## 8. Riskler ve Çözüm Önerileri

- **Risk:** Vulkan'ın karmaşıklığı nedeniyle yavaş ilerleme.  
  **Çözüm:** Her aşamada tek bir hedefe odaklanmak. Sorunları çözmek için bol bol loglama ve Vulkan Validation Layers'ı aktif kullanmak.  

- **Risk:** Motivasyon kaybı.  
  **Çözüm:** Yol haritasındaki küçük hedeflere ulaştıkça "küçük zaferleri" kutlamak. Projeyi düzenli olarak Git'e commit'leyerek ilerlemeyi somutlaştırmak.  

- **Risk:** Kapsamın kontrolsüz büyümesi (Scope Creep).  
  **Çözüm:** Bu brifing belgesindeki "Kapsam Dışı" listesine sadık kalmak. Yeni bir özellik eklemeden önce mevcut aşamanın hedeflerini tamamlamak.
