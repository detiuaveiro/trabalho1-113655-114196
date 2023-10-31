/// A generic instrumentation module.
///
/// João Manuel Rodrigues, AED, 2023
/// Code for cpu_time() by
/// Tomás Oliveira e Silva, AED, October 2021
///
/// Use as follows:
///
/// // Name the counters you're going to use: 
/// InstrName[0] = "memops";
/// InstrName[1] = "adds";
/// InstrCalibrate();  // Call once, to measure CTU
/// ...
/// InstrReset();  // reset to zero
/// for (...) {
///   InstrCount[0] += 3;  // to count array acesses
///   InstrCount[1] += 1;  // to count addition
///   a[k] = a[i] + a[j];
/// }
/// InstrPrint();  // to show time and counters

#ifndef INSTRUMENTATION_H
#define INSTRUMENTATION_H

/// Cpu time in seconds
double cpu_time(void) ; ///

/// 10 counters deve ser o bastante
#define NUMCOUNTERS 10

/// Array da operação dos counters:
extern unsigned long InstrCount[NUMCOUNTERS];  ///extern

/// Array dos nomes dos counters:
extern char* InstrName[NUMCOUNTERS];  ///extern

/// Cpu_time read on previous reset (~seconds)
extern double InstrTime;  ///extern

/// unidade de tempo calibrada (em segundos, inicialmente 1s)
extern double InstrCTU;  ///extern



/// Encontre a Unidade de Tempo Calibrada (UTC).
/// Execute e cronometre um loop de operações de memória e aritmética básicas
/// para definir uma unidade de tempo razoavelmente independente da CPU.
void InstrCalibrate(void) ;

/// Reset counters to zero and store cpu_time.
void InstrReset(void) ;

void InstrPrint(void) ;

#endif

