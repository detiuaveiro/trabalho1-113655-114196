/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec:  Name:
// 113655 Abel José Enes Teixeira
// 114196 Filipe Pina de Sousa
// 
// Date:
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// A estrutura de dados
//
// Uma imagem é armazenada em uma estrutura que contém 3 campos:
// Dois inteiros armazenam a largura e altura da imagem.
// O outro campo é um ponteiro para um array que armazena o nível de cinza de 8 bits
// de cada pixel na imagem. O array de pixels é unidimensional
// e corresponde a uma "varredura de matriz" da imagem da esquerda para a direita,
// de cima para baixo.
// Por exemplo, em uma imagem com 100 pixels de largura (img->width == 100),
// a posição do pixel (x, y) = (33,0) é armazenada em img->pixel[33];
// a posição do pixel (x, y) = (22,1) é armazenada em img->pixel[122].
//
// Os clientes devem usar imagens apenas por meio de variáveis do tipo Image,
// que são ponteiros para a estrutura de imagem, e não devem acessar os
// campos da estrutura diretamente.


//Valor máximo que você pode armazenar em um pixel (valor máximo aceitável)
const uint8 PixMax = 255;



// Estrutura interna para armazenar imagens em tons de cinza de 8 bits
struct image {
  int width;
  int height;
  int maxval;   // valor máximo de cinza (pixels com maxval são puros BRANCOS)
  uint8* pixel; // dados do pixel (uma varredura de matriz)
};



// Este módulo segue os princípios de "design by contract".
// Leia Design-by-Contract.md para obter mais detalhes.

/// Funções de tratamento de erro

// Neste módulo, apenas funções que lidam com alocação de memória ou operações de arquivo
// (I/O) usam técnicas defensivas.
//
// Quando uma dessas funções falha, ela sinaliza isso retornando um valor de erro
// como NULL ou 0 (veja a documentação da função), e define uma variável interna (errCause)
// para uma string que indica a causa da falha.
// A variável global errno, amplamente usada na biblioteca padrão, é
// cuidadosamente preservada e propagada, e os clientes podem usá-la juntamente com
// a função ImageErrMsg() para produzir mensagens de erro informativas.
// O uso da função error() da biblioteca padrão GNU é recomendado para
// esse propósito.
//
// Informações adicionais: man 3 errno; man 3 error;
static int errsave = 0;

// Causa do erro
static char* errCause;
/// Causa do erro.
/// Após uma falha de alguma outra função do módulo (e o retorno de um código de erro),
/// a chamada a esta função recupera uma mensagem apropriada descrevendo a
/// causa da falha. Isso pode ser usado junto com a variável global errno
/// para produzir mensagens de erro informativas (usando error(), por exemplo).
///
/// Após uma operação bem-sucedida, o resultado não é garantido (pode ser
/// a causa de erro anterior). Não se destina a ser usado nessa situação!
char* ImageErrMsg() { ///
  return errCause;
}


// Auxílios para programação defensiva
//
// A programação defensiva adequada em C, que não possui um mecanismo de exceção,
// geralmente resulta em possivelmente longas cadeias de chamadas de função, verificação de erros,
// código de limpeza e instruções de retorno:
// if (funA(x) == errorA) { return errorX; }
// if (funB(x) == errorB) { cleanupForA(); return errorY; }
// if (funC(x) == errorC) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Compreender essas cadeias é difícil e escrevê-las é entediante, confuso
// e propenso a erros. Os programadores tendem a ignorar os detalhes intrincados,
// e acabam produzindo programas inseguros e às vezes incorretos.
//
// Neste módulo, tentamos lidar com essas cadeias usando uma técnica um tanto
// não convencional. Isso recorre a uma função interna muito simples
// (verificar) que é usada para encapsular as chamadas de função e testes de erro e encadeá-los em uma longa expressão booleana
// que reflete o sucesso da operação como um todo:
// sucess =
// check(funA(x) != erro, "MsgFailA") &&
// check(funB(x) != erro, "MsgFailB") &&
// check(funC(x) != erro, "MsgFailC");
// if (!sucess) {
// códigoDeLimpezaCondicional();
// }
// return sucess;
//
// Quando uma função falha, a cadeia é interrompida, graças ao
// operador && de curto-circuito, e a execução pula para o código de limpeza.
// Enquanto isso, a função verificar() define errCause para uma mensagem apropriada.
//
// Esta técnica tem algumas questões de legibilidade e nem sempre é aplicável,
// mas é bastante concisa e concentra o código de limpeza em um único lugar.
//
// Veja exemplos de utilização em ImageLoad e ImageSave.
//
// (Você não é obrigado a usar isso em seu código!)

// Verifica uma condição e define errCause para failmsg em caso de falha.
// Isso pode ser usado para encadear uma sequência de operações e verificar seu sucesso.
// Propaga a condição.
// Preserva o erro global!
static int check(int condition, const char* failmsg) {
  errCause = (char*)(condition ? "" : failmsg);
  return condition;
}


/// Inicializar a biblioteca de imagens. (Chame apenas uma vez!)
/// Atualmente, simplesmente calibra a instrumentação e define os nomes dos contadores.
void ImageInit(void) { ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
  
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

/// Funções de gerenciamento de imagens

/// Criar uma nova imagem preta.
/// largura, altura: as dimensões da nova imagem.
/// maxval: o nível de cinza máximo (correspondente ao branco).
/// Requisitos: largura e altura devem ser não negativas, maxval > 0.
///
/// Em caso de sucesso, uma nova imagem é retornada.
/// (O chamador é responsável por destruir a imagem retornada!)
/// Em caso de falha, retorna NULL e errno/errCause são definidos adequadamente.
Image ImageCreate(int width, int height, uint8 maxval) { ///
  assert (width >= 0);
  assert (height >= 0);
  assert (0 < maxval && maxval <= PixMax);
  // Insert your code here!
}

/// Destruir a imagem apontada por (*imgp).
/// imgp: endereço de uma variável de tipo Image.
/// Se (*imgp) == NULL, nenhuma operação é realizada.
/// Garante: (*imgp) == NULL.
/// Não deve falhar e deve preservar o errno global/errCause.
void ImageDestroy(Image* imgp) { ///
  assert (imgp != NULL);
  // Insert your code here!
}


/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Encontre e pule 0 ou mais linhas de comentário no arquivo f.
// Comentários começam com um # e continuam até o final da linha, inclusive.
// Retorna o número de comentários pulados.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Carregar um arquivo PGM cru.
/// Somente arquivos PGM de 8 bits são aceitos.
/// Em caso de sucesso, uma nova imagem é retornada.
/// (O chamador é responsável por destruir a imagem retornada!)
/// Em caso de falha, retorna NULL e errno/errCause são definidos adequadamente.
Image ImageLoad(const char* filename) { ///
  int w, h;
  int maxval;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  int success = 
  check( (f = fopen(filename, "rb")) != NULL, "Open failed" ) &&
  // Parse PGM header
  check( fscanf(f, "P%c ", &c) == 1 && c == '5' , "Invalid file format" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &w) == 1 && w >= 0 , "Invalid width" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &h) == 1 && h >= 0 , "Invalid height" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax , "Invalid maxval" ) &&
  check( fscanf(f, "%c", &c) == 1 && isspace(c) , "Whitespace expected" ) &&
  // Allocate image
  (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
  // Read pixels
  check( fread(img->pixel, sizeof(uint8), w*h, f) == w*h , "Reading pixels" );
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL) fclose(f);
  return img;
}



/// Salvar a imagem em um arquivo PGM.
/// Em caso de sucesso, retorna um valor diferente de zero.
/// Em caso de falha, retorna 0, e errno/errCause são definidos apropriadamente,
/// e um arquivo parcial e inválido pode ser deixado no sistema
int ImageSave(Image img, const char* filename) { ///
  assert (img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE* f = NULL;

  int success =
  check( (f = fopen(filename, "wb")) != NULL, "Open failed" ) &&
  check( fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed" ) &&
  check( fwrite(img->pixel, sizeof(uint8), w*h, f) == w*h, "Writing pixels failed" ); 
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (f != NULL) fclose(f);
  return success;
}


/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///
  assert (img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img) { ///
  assert (img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///
  assert (img != NULL);
  return img->maxval;
}

/// Estatísticas de pixels
/// Encontra os níveis de cinza mínimo e máximo na imagem.
/// Após a execução,
/// *min é definido como o nível de cinza mínimo na imagem,
/// *max é definido como o máximo.
void ImageStats(Image img, uint8* min, uint8* max) { ///
  assert (img != NULL);
  // Insert your code here!
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
  assert (img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  // Insert your code here!
}

/// Pixel get & set operations
/// Estas são operações primitivas para acessar e modificar um único pixel
/// na imagem.
/// São operações muito simples, mas fundamentais, que podem ser usadas para
/// implementar operações mais complexas.

// Transforma as coordenadas (x, y) em um índice linear de pixel.
// Esta função interna é usada em ImageGetPixel / ImageSetPixel.
// O índice retornado deve satisfazer (0 <= índice < img->width*img->height)
static inline int G(Image img, int x, int y) {
  int index;
  // Insert your code here!
  assert (0 <= index && index < img->width*img->height);
  return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (read)
  return img->pixel[G(img, x, y)];
} 

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
} 


/// Pixel transformations

/// Essas funções modificam os níveis de pixel em uma imagem, mas não alteram
/// as posições dos pixels ou a geometria da imagem de nenhuma forma.
/// Todas essas funções modificam a imagem no local: nenhuma alocação é envolvida.
/// Elas nunca falham.

/// Transforma a imagem em uma imagem negativa.
/// Isso transforma pixels escuros em pixels claros e vice-versa,
/// resultando em um efeito de "negativo fotográfico".
void ImageNegative(Image img) { ///
  assert (img != NULL);
  // Insert your code here!
}


/// Aplicar um limite à imagem.
/// Transforma todos os pixels com nível<limiar em preto (0) e
/// todos os pixels com nível>=limiar em branco (maxval).
void ImageThreshold(Image img, uint8 thr) { ///
  assert (img != NULL);
  // Insert your code here!
}



/// Clarear a imagem por um fator.
/// Multiplica cada nível de pixel por um fator, mas satura em maxval.
/// Isso clareará a imagem se o fator for>1.0 e
/// escurecerá a imagem se o fator for<1.0.
void ImageBrighten(Image img, double factor) { ///
  assert (img != NULL);
  // ? assert (factor >= 0.0);
  // Insert your code here!
}

/// Transformações geométricas

/// Estas funções aplicam transformações geométricas a uma imagem,
/// retornando uma nova imagem como resultado.
///
/// Sucesso e falha são tratados como em ImageCreate:
/// Em caso de sucesso, uma nova imagem é retornada.
/// (O chamador é responsável por destruir a imagem retornada!)
/// Em caso de falha, retorna NULL e errno/errCause são definidos adequadamente.

// Dica de implementação:
// Chame ImageCreate sempre que precisar de uma nova imagem!

/// Girar uma imagem.
/// Retorna uma versão girada da imagem.
/// A rotação é de 90 graus no sentido horário.
/// Garante: A imagem original não é modificada.
///
/// Em caso de sucesso, uma nova imagem é retornada.
/// (O chamador é responsável por destruir a imagem retornada!)
/// Em caso de falha, retorna NULL e errno/errCause são definidos adequadamente.
Image ImageRotate(Image img) { ///
  assert (img != NULL);
  // Insert your code here!
}
/// Espelhar uma imagem = inverter da esquerda para a direita.
/// Retorna uma versão espelhada da imagem.
/// Garante: A imagem original não é modificada.
///
/// Em caso de sucesso, uma nova imagem é retornada.
/// (O chamador é responsável por destruir a imagem retornada!)
/// Em caso de falha, retorna NULL e errno/errCause são definidos adequadamente.
Image ImageMirror(Image img) { ///
  assert (img != NULL);
  // Insert your code here!
}
/// Recortar uma subimagem retangular de img.
/// O retângulo é especificado pelas coordenadas do canto superior esquerdo (x, y) e
/// largura w e altura h.
/// Requisitos:
/// O retângulo deve estar dentro da imagem original.
/// Garante:
/// A imagem original não é modificada.
/// A imagem retornada tem largura w e altura h.
///
/// Em caso de sucesso, uma nova imagem é retornada.
/// (O chamador é responsável por destruir a imagem retornada!)
/// Em caso de falha, retorna NULL e errno/errCause são definidos adequadamente.
Image ImageCrop(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  assert (ImageValidRect(img, x, y, w, h));
  // Insert your code here!
}



/// Colar uma imagem em uma imagem maior.
/// Cole img2 na posição (x, y) de img1.
/// Isso modifica img1 no local: nenhuma alocação é envolvida.
/// Requisitos: img2 deve caber dentro de img1 na posição (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));
  // Insert your code here!
}
/// Misturar uma imagem em uma imagem maior.
/// Misture img2 na posição (x, y) de img1.
/// Isso modifica img1 no local: nenhuma alocação é envolvida.
/// Requisitos: img2 deve caber dentro de img1 na posição (x, y).
/// Geralmente, o alpha está no intervalo de [0.0, 1.0], mas valores fora desse intervalo
/// podem proporcionar efeitos interessantes. Os estouros/subestouros devem saturar.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));
  // Insert your code here!
}
/// Comparar uma imagem a uma subimagem de uma imagem maior.
/// Retorna 1 (verdadeiro) se img2 corresponder à subimagem de img1 na posição (x, y).
/// Retorna 0, caso contrário.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidPos(img1, x, y));
  // Insert your code here!
}
/// Localizar uma subimagem dentro de outra imagem.
/// Procura por img2 dentro de img1.
/// Se uma correspondência for encontrada, retorna 1 e a posição correspondente é definida nas variáveis (*px, *py).
/// Se nenhuma correspondência for encontrada, retorna 0 e (*px, *py) permanecem inalterados.
int ImageLocateSubImage(Image img1, int* px, int* py, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  // Insert your code here!
}


/// Filtering
/// Desfocar uma imagem aplicando um filtro de média (2dx+1)x(2dy+1).
/// Cada pixel é substituído pela média dos pixels no retângulo
/// [x-dx, x+dx]x[y-dy, y+dy].
/// A imagem é alterada no local.
void ImageBlur(Image img, int dx, int dy) { ///
  // Insert your code here!
}

