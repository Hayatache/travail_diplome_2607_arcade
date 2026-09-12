#ifndef LM92_H
#define LM92_H

#include <stdint.h>


/**
 * @brief Lit la température du LM92.
 *
 * @return Température en degrés Celsius.
 */
float LM92_GetTemperature(void);

#endif /* LM92_H */