/*
*   ESP32 Template
*   Macro Handling
*/
#ifndef MACRO_HANDLING_H
#define MACRO_HANDLING_H

#define DOUBLEESCAPE(a) #a
#define TEXTIFY(a) DOUBLEESCAPE(a)

#endif //MACRO_HANDLING_H