# MMCG
Репозиторий для выполения задания по курсу "Современные методы компьютерной графики", весна 2021 года

## Зависимости

Все, кроме OpenGL и CMake скачается само.

- OpenGL, GLFW, glad
- CMake для сборки
- stb для загрузки изображений
- assimp для загрузки сцены
- nlohmann/json для работы с конфигом
- glm для матричных/векторных вычислений

## Сборка проекта

1. Установить [Git LFS](https://git-lfs.github.com/)
```
git lfs install
```
2. Склонировать репозиторий 
```
git clone --recurse-submodules git@github.com:shorohml/MMCG.git
```
или
```
git clone git@github.com:shorohml/MMCG.git
cd MMCG
git submodule update --init --recursive
```
3. Собрать проект
```
mkdir -p build && cd build && cmake .. && make -j 5
``` 
4. Запустить приложение
```
./main
```

## Выполненные пункты задания

- База # 1
- База # 2
- VSM (variance shadow map): от + 2 до +4 балов
- Тени от источника во все стороны (при помощи кубических текстурных карт): +4
- Применение карт освещённости (diffuse, specular, normal map) (+3 балла)
- Bloom (или другая пост-обработка для HDR, +2)
- Реалистичные движения (анимация персонажей) или физическая симуляция: (от 1 до
4 баллов)
