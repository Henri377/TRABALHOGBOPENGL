# TRABALHOGBOPENGL

# Jogo de Tiles Isométricos com Moedas e Bandeira

Este projeto é um jogo simples em C++ usando OpenGL, GLFW, GLAD, GLM e stb_image. O jogador pode mover um personagem por um mapa isométrico, coletar moedas animadas e alcançar uma bandeira animada para vencer o jogo.

## Como funciona

- O mapa é carregado a partir de um arquivo `map.txt`.
- Os tiles que não podem ser atravessados são definidos em `tiles_bloqueados.txt`.
- O personagem, moedas e bandeira são sprites animados.
- O objetivo é coletar moedas e chegar até a bandeira.

## Estrutura dos arquivos

- **map.txt**  
  Define o tileset, tamanho do mapa e a matriz de tiles.  
  Exemplo:
  ```
  tilesetIso.png 7 57 114
  10 8
  0 0 1 1 1 1 1 1 0 0
  0 1 2 2 2 2 2 2 1 0
  ...
  ```

- **tiles_bloqueados.txt**  
  Lista os índices dos tiles que são obstáculos (um por linha ou separados por espaço):
  ```
  3 4 6
  ```

- **Sprites e imagens**  
  - `personagem_spritesheet.png` — Sprite do personagem
  - `coin_Sheet.png` — Sprite das moedas
  - `flag animation.png` — Sprite da bandeira
  - `tilesetIso.png` — Tileset do mapa

## Como compilar

Certifique-se de ter as bibliotecas **GLFW**, **GLAD**, **GLM** e **stb_image** disponíveis no seu projeto.

Exemplo de compilação (ajuste os caminhos conforme necessário):

```sh
g++ trabalhogb.cpp -o jogo -lglfw3 -lopengl32 -lgdi32
```

## Como jogar

- Use as teclas **W, A, S, D, Q, E, Z, X** para mover o personagem pelo mapa.
- Colete moedas passando por cima delas.
- Alcance a bandeira para finalizar o jogo.

## Organização do código

- O código está dividido em funções para carregar o mapa, desenhar elementos, lidar com colisões e controlar o personagem.
- A lógica de colisão usa a distância entre sprites.
- Tiles não caminháveis são lidos de um arquivo externo.
