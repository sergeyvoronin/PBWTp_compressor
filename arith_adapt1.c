/*
 arith_adapt.cpp
 Адаптивное арифметическое кодирование.
 Демонстративная программа.
 Использование:
   arith_adapt e(ncode)|d(ecode) infile outfile

with buffered in/out
*/

#include <stdio.h>
#include <stdlib.h>
//#include <process.h>

// Количество битов в регистре
#define BITS_IN_REGISTER 16

// Максимально возможное значение в регистре
#define TOP_VALUE (((long) 1 << BITS_IN_REGISTER) - 1)

// Диапазоны
#define FIRST_QTR (TOP_VALUE / 4 + 1)
#define HALF (2 * FIRST_QTR)
#define THIRD_QTR (3 * FIRST_QTR)

// Количество символов алфавита
#define NO_OF_CHARS 256
// Специальный символ КонецФайла
#define EOF_SYMBOL    (NO_OF_CHARS + 1)
// Всего символов в модели
#define NO_OF_SYMBOLS (NO_OF_CHARS + 1)

// Порог частоты для масштабирования
#define MAX_FREQUENCY 16383

// Таблицы перекодировки
unsigned char index_to_char [NO_OF_SYMBOLS];
int char_to_index [NO_OF_CHARS];

// Таблицы частот
int cum_freq [NO_OF_SYMBOLS + 1];
int freq [NO_OF_SYMBOLS + 1];

// Регистры границ и кода
long low, high;
long value;

// Поддержка побитлвых операций с файлами
long bits_to_follow;
int bufvar;
int bits_to_go;
int garbage_bits;

// Обрабатываемые файлы
FILE *in, *out;


// buffer vars
#define MAX 40980
unsigned char buffer;
unsigned char inputBuffer [MAX];
unsigned char *currentPtr = 0;
int dataLeft = 0;
int rc ;
unsigned char outputBuffer [MAX];
int outputLength = 0;



/* ******************************************************* */
/* GetChar - retrieve a char a time with a large buffer    */
/*                                                         */
/* ******************************************************* */
char getChar(FILE *fd, unsigned char *ch)
{

      if (dataLeft == 0 ) {
	      rc = fread(inputBuffer, 1, MAX, fd);
	
		  if (rc >0 ) {
		      currentPtr = inputBuffer;
			  dataLeft = rc;
		  } else {
		    // no more data
		    return -1;
		  }
	  }
	  *ch = (char)*currentPtr++;
	  dataLeft--;
 
	return 1;
 }
 
/* ************************************************************ */
/* flushOutput - Flush out the output buffer                    */                                                   
/* ************************************************************ */
 void flushOutput(FILE *fd){
   if (outputLength > 0) 
      fwrite(outputBuffer, 1, outputLength, fd);
   return;
 }
 
/* ************************************************************ */
/* putChar - output a char a time with a large buffer           */
/* must call the fluchOutput(FILE *fd) to flush out the buffer  */                                                    
/* ************************************************************ */
int putChar(FILE *out, unsigned char ch)
{
 
      if (outputLength >= MAX ) {
	      rc = fwrite(outputBuffer, 1, MAX, out);
		   //printf("outputLength = %d, rc=%d\n", outputLength, rc);
		  if (rc >=0 ) {
		      outputLength = 0;
		
		  } else {
		    // no more data
		    return -1;
		  }
	  }
	  outputBuffer[outputLength] = ch;
	  outputLength++;
  
	return 1;
}



//------------------------------------------------------------
// Инициализация адаптивной модели
void start_model (void)
{
  int i;

  for ( i = 0; i < NO_OF_CHARS; i++)
  {
    char_to_index [i] = i + 1;
    index_to_char [i + 1] = i;
  }
  for ( i = 0; i <= NO_OF_SYMBOLS; i++)
  {
    freq [i] = 1;
    cum_freq [i] = NO_OF_SYMBOLS - i;
  }
  freq [0] = 0;
}

//------------------------------------------------------------
// Обновление модели очередным символом
void update_model ( int symbol)
{
  int i;
  int ch_i, ch_symbol;
  int cum;

  // проверка на переполнение счетчика частоты
  if (cum_freq [0] == MAX_FREQUENCY)
  {
    cum = 0;
    // масштабирование частот при переполнении
    for ( i = NO_OF_SYMBOLS; i >= 0; i--)
    {
      freq [i] = (freq [i] + 1) / 2;
      cum_freq [i] = cum;
      cum += freq [i];
    }
  }

  for ( i = symbol; freq [i] == freq [i - 1]; i--);
  if (i < symbol)
  {
    ch_i                      = index_to_char [i];
    ch_symbol                 = index_to_char [symbol];
    index_to_char [i]         = ch_symbol;
    index_to_char [symbol]    = ch_i;
    char_to_index [ch_i]      = symbol;
    char_to_index [ch_symbol] = i;
  }

  // обновление значений в таблицах частот
  freq [i] += 1;
  while (i > 0)
  {
    i -= 1;
    cum_freq [i] += 1;
  }
}

//------------------------------------------------------------
// Инициализация побитового ввода
void start_inputing_bits (void)
{
  bits_to_go = 0;
  garbage_bits = 0;
}

//------------------------------------------------------------
// Ввод очередного бита сжатой информации
int input_bit (void)
{
  int t,ch;

  if (bits_to_go == 0)
  {
    //buffer = getc (in);
    rc = getChar(in, &buffer); 
    bufvar = (int) buffer; 

    if (bufvar == EOF)
    {
      garbage_bits += 1;
      if (garbage_bits > BITS_IN_REGISTER - 2)
      {
 printf ("Ошибка в сжатом файле\n");
 exit (-1);
      }
    }
    bits_to_go = 8;
  }
  t = bufvar & 1;
  bufvar >>= 1;
  bits_to_go -= 1;
  return t;
}

//------------------------------------------------------------
// Инициализация побитового вывода
void start_outputing_bits (void)
{
  bufvar = 0;
  bits_to_go = 8;
}

//------------------------------------------------------------
// Вывод очередного бита сжатой информации
void output_bit ( int bit)
{
  bufvar >>= 1;
  if (bit)
    bufvar |= 0x80;
  bits_to_go -= 1;
  if (bits_to_go == 0)
  {
    //putc ( buffer, out);
	putChar(out,bufvar);
    bits_to_go = 8;
  }
}

//------------------------------------------------------------
// Очистка буфера побитового вывода
void done_outputing_bits (void)
{
  //putc ( buffer >> bits_to_go, out);
	putChar(out,bufvar >> bits_to_go);
}

//------------------------------------------------------------
// Вывод указанного бита и отложенных ранее
void output_bit_plus_follow ( int bit)
{
  output_bit (bit);
  while (bits_to_follow > 0)
  {
    output_bit (!bit);
    bits_to_follow--;
  }
}

//------------------------------------------------------------
// Инициализация регистров границ и кода перед началом сжатия
void start_encoding (void)
{
  low            = 0l;
  high           = TOP_VALUE;
  bits_to_follow = 0l;
}

//------------------------------------------------------------
// Очистка побитового вывода
void done_encoding (void)
{
  bits_to_follow++;
  if (low < FIRST_QTR)
    output_bit_plus_follow (0);
  else
    output_bit_plus_follow (1);
}

//------------------------------------------------------------
/* Инициализация регистров перед декодированием.
   Загрузка начала сжатого сообщения
*/
void start_decoding (void)
{
  int i;

  value = 0l;
  for ( i = 1; i <= BITS_IN_REGISTER; i++)
    value = 2 * value + input_bit ();
  low = 0l;
  high = TOP_VALUE;
}


//------------------------------------------------------------
// Кодирование очередного символа
void encode_symbol(int symbol)
{
	static const unsigned // to save on macro unwinding
		Half = HALF,
		FirstQtr = FIRST_QTR,
		ThirdQtr = THIRD_QTR;
	// пересчет значений границ
	const long range = high - low + 1;
	const int cumFreq0 = cum_freq[0];
	high = low + (range * cum_freq[symbol - 1]) / cumFreq0 - 1;
	low = low + (range * cum_freq[symbol]) / cumFreq0;
	// выдвигание очередных битов
	for (;;)
	{
		if (high < Half)
			output_bit_plus_follow(0);
		else if (low >= Half)
		{
			output_bit_plus_follow(1);
			low -= Half;
			high -= Half;
		}
		else if (low >= FirstQtr && high < ThirdQtr)
		{
			bits_to_follow += 1;
			low -= FirstQtr;
			high -= FirstQtr;
		}
		else break;
		// сдвиг влево с "втягиванием" очередного бита
		low += low; // low = 2 * low; // multiplication is more expensive
		high += high; ++high; // high = 2 * high + 1; //
	}
}

//------------------------------------------------------------
// Декодирование очередного символа
int decode_symbol (void)
{
  long range;
  int cum, symbol;

  // определение текущего масштаба частот
  range = (long) (high - low) + 1;
  // масштабирование значения в регистре кода
  cum = (int)
    ((((long) (value - low) + 1) * cum_freq [0] - 1) / range);
  // поиск соответствующего символа в таблице частот
  for (symbol = 1; cum_freq [symbol] > cum; symbol++);
  // пересчет границ
  high = low + (range * cum_freq [symbol - 1]) / cum_freq [0] - 1;
  low = low + (range * cum_freq [symbol]) / cum_freq [0];
  // удаление очередного символа из входного потока
  for (;;)
  {
    if (high < HALF)
    {
    }
    else if (low >= HALF)
    {
      value -= HALF;
      low -= HALF;
      high -= HALF;
    }
    else if (low >= FIRST_QTR && high < THIRD_QTR)
    {
      value -= FIRST_QTR;
      low -= FIRST_QTR;
      high -= FIRST_QTR;
    }
    else
      break;
    // сдвиг влево с "втягиванием очередного бита
    low = 2 * low;
    high = 2 * high + 1;
    value = 2 * value + input_bit ();
  }
  return symbol;
}

//------------------------------------------------------------
// Собственно адаптивное арифметическое кодирование
void encode ( char *infile, char *outfile)
{
  int ch, symbol;

  in = fopen ( infile, "r+b");
  out = fopen ( outfile, "w+b");
  if (in == NULL || out == NULL)
    return;
  start_model ();
  start_outputing_bits ();
  start_encoding ();
  for (;;)
  {
    rc = getChar(in, &buffer); 
    if (rc == -1)
        break;
    ch = (int) buffer; 
    /*ch = getc (in);
    if (ch == EOF)
      break;*/
    symbol = char_to_index [ch];
    encode_symbol (symbol);
    update_model (symbol);
  }
  encode_symbol (EOF_SYMBOL);
  done_encoding ();
  done_outputing_bits ();
  flushOutput(out);
  fclose (in);

  fclose (out);
}

//------------------------------------------------------------
// Собственно адаптивное арифметическое декодирование
void decode ( char *infile, char *outfile)
{
  int ch, symbol;

  in = fopen ( infile, "r+b");
  out = fopen ( outfile, "w+b");
  if (in == NULL || out == NULL)
    return;
  start_model ();
  start_inputing_bits ();
  start_decoding ();
  for (;;)
  {
    symbol = decode_symbol ();
    if (symbol == EOF_SYMBOL)
      break;
    ch = index_to_char [symbol];
    //putc ( ch, out);
    putChar(out,ch);
    update_model (symbol);
  }
  fclose (in);
    
  flushOutput(out);
  fclose (out);
}

//------------------------------------------------------------
// Головная процедура
int main(int argc, char* argv[])
{
 if (argc < 4)
  {
    printf ("\n Using: Arith_adapt e|d infile outfile\n");
    exit (0);
  }
  if (argv [1] [0] == 'e')
    encode ( argv [2], argv [3]);
  else if (argv [1] [0] == 'd')
    decode ( argv [2], argv [3]);
  exit (0);

 return 0;
}

