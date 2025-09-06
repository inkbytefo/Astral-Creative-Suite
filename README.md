# Astral Engine

Astral Engine, C++17 ve Vulkan API kullanılarak sıfırdan geliştirilen modern bir 3D oyun motorudur.

## Proje Durumu

**Mevcut Aşama:** Aşama 0: Kurulum ve Temel Altyapı

## Gereksinimler

- CMake 3.16 veya üzeri
- C++17 destekli bir derleyici (MSVC önerilir)
- Vulkan SDK
- SDL3 kütüphanesi (external/SDL3 dizininde olmalıdır)

## Kurulum ve Derleme

### Windows (MSVC - Önerilen)

1. Bu depoyu klonlayın veya indirin
2. Gerekli harici kütüphaneleri (SDL3, GLM, etc.) `external` dizinine yerleştirin
3. "Developer Command Prompt for VS 2022" komut istemini açın
4. Proje dizinine gidin:
   ```cmd
   cd C:\Users\tpoyr\OneDrive\Desktop\AstralEngine
   ```
5. Derleme komutlarını çalıştırın:
   ```cmd
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022"
   cmake --build . --config Release
   ```

Alternatif olarak, proje dizinindeki `build_msvc.bat` dosyasını çalıştırabilirsiniz.

### Diğer Platformlar

Diğer platformlarda derleme yapmak için CMake'i uygun derleyici ayarlarıyla çalıştırın.