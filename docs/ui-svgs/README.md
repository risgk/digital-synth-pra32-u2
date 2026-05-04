# ST7789 UI SVG mocks (284x76)

Estes SVGs demonstram a interface colorida do display ST7789 de forma fiel ao layout definido em `pra32-u2-ui-render-st7789.cpp`:

- resolução lógica `284x76`
- header em `y=0..13`
- 3 cards de `90x42` em `y=18`
- rodapé em `y=64..75`
- faixa de confirmação em `y=60..63`
- paleta baseada nas cores RGB565 do renderer

Arquivos:

- `st7789-interface-nav.svg` — estado de navegação
- `st7789-interface-edit.svg` — estado de edição
- `st7789-interface-confirm.svg` — estado de confirmação de ação perigosa
