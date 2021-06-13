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

Проверял сборку только под Linux.

1. Установить [Git LFS](https://git-lfs.github.com/)
```
sudo apt-get install git-lfs && git lfs install
```
2. Склонировать репозиторий
```
git clone --recurse-submodules git@github.com:shorohml/MMCG.git && cd MMCG
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
- Bloom (+2)
- Физическая симуляция: (от 1 до 4 баллов)
- MSAA (+2 балла)

## Управление

- WASD - перемещение
- 1 - отрисовка по умолчанию
- 2 - визуализация буфера глубины
- 3 - визуализация нормалей (цветом)
- space - wireframe

## Результат

Должно выглядеть примерно так

![](data/appScreenshot.png)
