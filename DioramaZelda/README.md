# Diorama Zelda — Visualizador Final 3D

Visualizador unificado (Computação Gráfica / Unisinos) que integra todo o
pipeline da disciplina numa única cena (diorama de Hyrule): carregamento de
malhas, geometria procedural, materiais `.mtl`, texturas, iluminação de Phong
com 3 luzes, câmera FPS, seleção/transformação de objetos e animação por curva
de Bézier.

## Como compilar e rodar

A partir da raiz do repositório:

```bash
cmake -S . -B build
cmake --build build --target DioramaZelda

cd build
./DioramaZelda          # Windows: .\DioramaZelda.exe
```

> **Importante:** execute de dentro de `build/`. Os caminhos de assets são
> relativos a essa pasta (`../assets/...` e `../DioramaZelda/assets/cena.txt`).

## Controles

| Tecla | Ação |
|-------|------|
| `W A S D` + mouse | Navegar com a câmera (FPS) |
| `TAB` / `1`–`9` | Selecionar objeto (próximo / por índice) |
| `J`/`L`, `I`/`K`, `U`/`O` | Transladar o objeto selecionado (X, Z, Y) |
| `R` / `T` | Rotacionar o objeto selecionado (eixo Y) |
| `+` / `-` | Escala uniforme | 
| `0` | Resetar transformações do objeto |
| `F1` `F2` `F3` | Ligar/desligar luzes **key / fill / back** |
| `G` | Trocar a luz ativa; `[` `]` ajustam a intensidade dela |
| `M` | Alternar **textura ↔ cor do material (`.mtl`)** |
| `ESPAÇO` | Play/pause da animação de Bézier |
| `ESC` | Sair |

## Cena (`assets/cena.txt`)

Arquivo de configuração lido em tempo de execução. Formato:

```
light  <nome> on|off  px py pz   r g b   intensidade
object <nome> <geom>  <mtl|-|COL:r,g,b>  <tex|->  px py pz  escala  [spin]
waypoint x y z      # anexa ponto da trajetória de Bézier ao último objeto
```

`<geom>` pode ser um `.obj` ou geometria procedural: `PROC:ground`,
`PROC:triforce`, `PROC:rupee`, `PROC:cube`, `PROC:sphere`.

## Onde está cada cálculo (guia da arguição)

| Item da rubrica | Arquivo / função |
|-----------------|------------------|
| Parser do arquivo de cena | `src/main.cpp` → `loadScene()` |
| Parser do material `.mtl` (Ka/Kd/Ks/Ns) | `src/Material.h` → `loadMTL()` |
| Carregamento de malha `.obj` (com normais) | `src/Mesh.h` → `loadSimpleOBJ()` |
| Geometria procedural | `src/Mesh.h` → `makeTriforce()`, `makeRupee()`, … |
| Passagem de uniforms | `src/main.cpp` → loop principal e `desenhaObjeto()` |
| Matriz de **Model** (T·R·S) | `src/main.cpp` → `montaModel()` |
| Matriz de **View** (câmera) | `src/Camera.h` → `getViewMatrix()` |
| Matriz de **Projeção** | `src/main.cpp` → `glm::perspective` no loop |
| Iluminação de Phong (3 luzes) | `src/main.cpp` → **Fragment Shader** |
| Curva de Bézier | `src/Bezier.h` → `bezierCubic()` / `evalClosedPath()` |

## Objetos do diorama

- **Chão** — quad procedural com grama de Hyrule.
- **Triforce** — 3 triângulos dourados coplanares, com giro automático.
- **Baú** — cubo procedural com textura (`pixelWall.png`).
- **Boss (Suzanne)** — malha `.obj` com material `.mtl` (Ka/Ks/Ns) + textura.
- **Rupia** — octaedro verde percorrendo uma curva de Bézier fechada.
