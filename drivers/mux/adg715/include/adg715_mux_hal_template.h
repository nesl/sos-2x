/**
 * @brief example hardware abstraction layer for the ADG715
 * @author Naim Busek <ndbusek@gmail.com>
 */

/**
 * to use this component your platform must define the user confgurable
 * bits in the address
 *
 * this is done by creating a adg715_mux_hal.h file in your platform
 * directory that defines the following necessary hardware abstractions
 *
 * in the case of the mux the only necessary abstraction is the address
 * please use the values defined in the device header file when defining
 * these values
 */

#define ADG715_MUX_ADDR   (ADG715_I2C_ADDR_A0 | ADG715_I2C_ADDR_A0)

