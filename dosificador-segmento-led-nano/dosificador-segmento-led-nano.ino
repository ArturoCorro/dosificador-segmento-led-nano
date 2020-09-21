/*
	Dosificador sin pantall

	Por Arturo Corro
	20 de Marzo del 2020
*/

#include <EEPROM.h>


/**************************************************
* Definición
**************************************************/
	/* Definición de Hardware
	**************************************************/
		#define SEGMENT_A A4
		#define SEGMENT_B A3
		#define SEGMENT_C 5
		#define SEGMENT_D 4
		#define SEGMENT_E 6
		#define SEGMENT_F A5
		#define SEGMENT_G 7

		#define MODE_BUTTON_PIN	  A2
		#define DELAY_BUTTON_PIN  A1
		#define DOSAGE_BUTTON_PIN A0

		#define SENSOR_PIN  3
		#define SSR_PIN 	2

		#define EEPROM_ADDRESS_CONTROLLER_MODE	0
		#define EEPROM_ADDRESS_DELAY_LEVEL		1
		#define EEPROM_ADDRESS_DOSAGE_LEVEL		2


	/* Definición de Setup
	**************************************************/
		#define BUTTON_PRESSED_DELAY_MILLIS	200

		#define BUTTON_STATUS_REPOSE			0
		#define BUTTON_STATUS_COMMAND_EXECUTED	1

		#define NO_BUTTON_PRESSED			0
		#define DOSING_MODE_BUTTON_PRESSED	1
		#define DELAY_LEVEL_BUTTON_PRESSED	2
		#define DOSAGE_LEVEL_BUTTON_PRESSED	3

		#define SETUP_OUT_OFF	   0
		#define SETUP_DOSAGE_MODE  1
		#define SETUP_DELAY_LEVEL  2
		#define SETUP_DOSAGE_LEVEL 3

		#define DELAY_MIN_LEVEL 0
		#define DELAY_MAX_LEVEL 9

		#define DOSAGE_MIN_LEVEL 0
		#define DOSAGE_MAX_LEVEL 9

		#define AUTOMATIC_CLOSING_SETTINGS_MILLIS 5000

		#define PRECIONADO	  0
		#define NO_PRECIONADO 1


	/* Definición Sensor
	**************************************************/
		#define SENSOR_SENSITIVITY_MILLIS 200

		#define BLOQUEADO LOW

		#define SENSOR_RESTING	0
		#define SENSOR_BLOCKED	1
		#define SENSOR_ACTIVE	2


	/* Definición Dosificador
	**************************************************/
		#define MODO_DISPENSER		0
		#define MODO_CONVEYOR_BELT	1

		#define DELAY_TIME_STEP_MILLIS	400

		#define DOSAGE_TIME_INIT_MILLIS	750
		#define DOSAGE_TIME_STEP_MILLIS	400

		#define MAX_DOSING_TIME_MILLIS 60000

		#define DOSING_TIMEOUT_MILLIS 400

		#define CONVEYOR_BELT_BUFFER_SIZE 10

		#define DOSAGE_RESTING			 0
		#define DOSAGE_IN_DELAY			 1
		#define DOSAGE_DOSING			 2
		#define DOSAGE_WAITING_UNBLOCKED 3
		#define DOSAGE_TIMEOUT 			 4

		#define ENCENDIDO 1
		#define APAGADO	  0


	/* Debug
	**************************************************/
		/**
		 * Define DEBUG_SERIAL_ENABLE to enable debug serial.
		 * Comment it to disable debug serial.
		 */
		//#define DEBUG_SERIAL_ENABLE

		/**
		 * Define dbSerial for the output of debug messages.
		 */
		#define dbSerial Serial

		#ifdef DEBUG_SERIAL_ENABLE
			#define serialPrint(a)    dbSerial.print(a)
			#define serialPrintln(a)  dbSerial.println(a)
			#define serialBegin(a)    dbSerial.begin(a)

		#else
			#define serialPrint(a)    do{}while(0)
			#define serialPrintln(a)  do{}while(0)
			#define serialBegin(a)    do{}while(0)
		#endif

		String dosage_string[5] = {
			"DOSAGE_RESTING",
			"DOSAGE_IN_DELAY",
			"DOSAGE_DOSING",
			"DOSAGE_WAITING_UNBLOCKED",
			"DOSAGE_TIMEOUT",
		};




/**************************************************
* Variables de la aplicación
**************************************************/
	/* Configuraciones
	**************************************************/
		uint8_t  button_push_status;
	 	uint32_t button_push_millis;
	 	uint32_t last_pushbutton_millis;

	 	uint8_t in_setup;

		uint8_t dosage_mode;
		uint8_t delay_level;
		uint8_t dosage_level;

		// Timmers
		uint32_t _T[2];


	/* Sensor
	**************************************************/
		uint8_t sensor_status;
		uint8_t last_sensor_status;
		uint32_t sensor_blocked_millis;


	/* Dosificador
	**************************************************/
		uint8_t dosage_status;
		uint32_t dosage_last_change_millis;

		uint32_t active_start_time_millis[CONVEYOR_BELT_BUFFER_SIZE];
		uint32_t active_end_time_millis[CONVEYOR_BELT_BUFFER_SIZE];

		uint8_t  active_sensor_idx;
		uint8_t  active_ssr_idx;

		uint32_t update_sensor_blocked_millis;
		uint32_t update_dosage_last_change_millis;


/**************************************************
* Métdodos del dosificador
**************************************************/
	/* Helpers Configuraciones
	**************************************************/
		void enter_a_setting(uint8_t param_setting)
		{
			in_setup = param_setting;

			switch(in_setup){
				case SETUP_DOSAGE_MODE:
					displayDigit(dosage_mode);
					break;

				case SETUP_DELAY_LEVEL:
					displayDigit(delay_level);
					break;

				case SETUP_DOSAGE_LEVEL:
					if(dosage_mode == MODO_CONVEYOR_BELT){
						turnOffAllSegments();
						digitalWrite(SEGMENT_G, HIGH);

					}else
						displayDigit(dosage_level);
					break;
			}

			serialPrintln("Entro a configuraciones");
			debugSetup();
		}

		void automatic_closed_settings()
		{
			if(in_setup != SETUP_OUT_OFF && last_pushbutton_millis + AUTOMATIC_CLOSING_SETTINGS_MILLIS <= millis()){
				in_setup = SETUP_OUT_OFF;
				last_pushbutton_millis = 0;

				turnOffAllSegments();

				serialPrintln("Salio de configuraciones");
				debugSetup();
			}
		}


	/* Display 7 segmentos
	**************************************************/
		void displayDigit(uint8_t digit)
		{
			turnOffAllSegments();

			//Conditions for displaying segment a
			if(digit != 1 && digit != 4)
				digitalWrite(SEGMENT_A, HIGH);

			//Conditions for displaying segment b
			if(digit != 5 && digit != 6)
				digitalWrite(SEGMENT_B, HIGH);

			//Conditions for displaying segment c
			if(digit != 2)
				digitalWrite(SEGMENT_C, HIGH);

			//Conditions for displaying segment d
			if(digit != 1 && digit != 4 && digit != 7)
				digitalWrite(SEGMENT_D, HIGH);

			//Conditions for displaying segment e
			if(digit == 2 || digit == 6 || digit == 8 || digit == 0)
				digitalWrite(SEGMENT_E, HIGH);

			//Conditions for displaying segment f
			if(digit != 1 && digit != 2 && digit != 3 && digit != 7)
				digitalWrite(SEGMENT_F, HIGH);

			if (digit != 0 && digit != 1 && digit != 7)
				digitalWrite(SEGMENT_G, HIGH);
		}

		void turnOffAllSegments()
		{
			digitalWrite(SEGMENT_A, LOW);
			digitalWrite(SEGMENT_B, LOW);
			digitalWrite(SEGMENT_C, LOW);
			digitalWrite(SEGMENT_D, LOW);
			digitalWrite(SEGMENT_E, LOW);
			digitalWrite(SEGMENT_F, LOW);
			digitalWrite(SEGMENT_G, LOW);
		}


	/* Cambiar configuraciónes del dosificador
	**************************************************/
		void switch_to_next_mode()
		{
			dosage_mode = !dosage_mode;

			EEPROM.write(EEPROM_ADDRESS_CONTROLLER_MODE, dosage_mode);

			displayDigit(dosage_mode);

			serialPrintln("switch_to_next_mode()");
			debugSetup();
		}

		void switch_to_the_next_level_of_delay()
		{
			delay_level = delay_level + 1 > DELAY_MAX_LEVEL? DELAY_MIN_LEVEL: delay_level +1;

			EEPROM.write(EEPROM_ADDRESS_DELAY_LEVEL, delay_level);

			displayDigit(delay_level);

			serialPrintln("switch_to_the_next_level_of_delay()");
			debugSetup();
		}

		void switch_to_the_next_dosage_level()
		{
			if(dosage_mode == MODO_CONVEYOR_BELT){
				turnOffAllSegments();
				digitalWrite(SEGMENT_G, HIGH);

			}else{
				dosage_level = dosage_level + 1 > DOSAGE_MAX_LEVEL? DOSAGE_MIN_LEVEL: dosage_level +1;

				EEPROM.write(EEPROM_ADDRESS_DOSAGE_LEVEL, dosage_level);

				displayDigit(dosage_level);

				serialPrintln("switch_to_the_next_dosage_level()");
			}

			debugSetup();
		}


	/* Obtener Configuraciones del dosificador
	**************************************************/
		uint8_t get_dosage_mode()
		{
			return EEPROM.read(EEPROM_ADDRESS_CONTROLLER_MODE);
		}

		uint32_t get_delay_level()
		{
			return EEPROM.read(EEPROM_ADDRESS_DELAY_LEVEL);
		}

		uint32_t get_dosing_level()
		{
			return EEPROM.read(EEPROM_ADDRESS_DOSAGE_LEVEL);
		}


	/* Inicializar
	**************************************************/
		void deviceInit()
		{
			serialBegin(9600);

			setupInit();
			dosageInit();

			serialPrintln("inicializo el Dosificador");
			debugSetup();
		}

		void setupInit()
		{
			pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
			pinMode(DELAY_BUTTON_PIN, INPUT_PULLUP);
			pinMode(DOSAGE_BUTTON_PIN, INPUT_PULLUP);

			pinMode(SEGMENT_A, OUTPUT);
			pinMode(SEGMENT_B, OUTPUT);
			pinMode(SEGMENT_C, OUTPUT);
			pinMode(SEGMENT_D, OUTPUT);
			pinMode(SEGMENT_E, OUTPUT);
			pinMode(SEGMENT_F, OUTPUT);
			pinMode(SEGMENT_G, OUTPUT);

			turnOffAllSegments();
		}

		void dosageInit()
		{
			pinMode(SENSOR_PIN, INPUT_PULLUP);
			pinMode(SSR_PIN, OUTPUT);

			// Cargamos configuraciones de EEPROM
			dosage_mode  = get_dosage_mode();
			delay_level  = get_delay_level();
			dosage_level = get_dosing_level();
		}



	/* Loops
	**************************************************/
		void deviceLoop()
		{
			setupLoop();
			sensorLoop();
			bufferDosageLoop();
			dosageLoop();
		}

		void setupLoop()
		{
			// ningun boton precionado
			if(digitalRead(MODE_BUTTON_PIN) == PRECIONADO){
				if(button_push_millis == 0)
					button_push_millis = millis() + 100;

				if(button_push_status == BUTTON_STATUS_REPOSE){
					if(button_push_millis <= millis()){
						if(in_setup == SETUP_DOSAGE_MODE)
							switch_to_next_mode();
						else
							enter_a_setting(SETUP_DOSAGE_MODE);

						button_push_status = BUTTON_STATUS_COMMAND_EXECUTED;
						last_pushbutton_millis = millis();
					}
				}

			}else if(digitalRead(DELAY_BUTTON_PIN) == PRECIONADO){
				// Cambiamos nivel de retrazo
				if(button_push_millis == 0)
					button_push_millis = millis() + 100;

				if(button_push_status == BUTTON_STATUS_REPOSE){
					if(button_push_millis <= millis()){
						if(in_setup == SETUP_DELAY_LEVEL)
							switch_to_the_next_level_of_delay();
						else
							enter_a_setting(SETUP_DELAY_LEVEL);

						button_push_status = BUTTON_STATUS_COMMAND_EXECUTED;
						last_pushbutton_millis = millis();
					}
				}

			}else if(digitalRead(DOSAGE_BUTTON_PIN) == PRECIONADO){
				// Cambiamos nivel de dosificación
				if(button_push_millis == 0)
					button_push_millis = millis() + 100;

				if(button_push_status == BUTTON_STATUS_REPOSE){
					if(button_push_millis <= millis()){
						if(in_setup == SETUP_DOSAGE_LEVEL)
							switch_to_the_next_dosage_level();

						else
							enter_a_setting(SETUP_DOSAGE_LEVEL);

						button_push_status = BUTTON_STATUS_COMMAND_EXECUTED;
						last_pushbutton_millis = millis();
					}
				}

			}else{
				// Reseteo de estatus de botones
				button_push_status = BUTTON_STATUS_REPOSE;
				button_push_millis = 0;
			}

			// Proceso para salir de configuraciones
			if(_T[0] <= millis()){
				_T[0] += 1000;

				automatic_closed_settings();

				debugDosage();
			}
		}

		void sensorLoop()
		{
			last_sensor_status = sensor_status;

			if(digitalRead(SENSOR_PIN) == BLOQUEADO){
				// Se acaba de Bloquear el Sensor
				if(sensor_status == SENSOR_RESTING){
					sensor_status		  = SENSOR_BLOCKED;
					sensor_blocked_millis = millis();
				}

				// El sensor supero el mínimo de tiempo bloqueado
				if(sensor_status == SENSOR_BLOCKED && sensor_blocked_millis + SENSOR_SENSITIVITY_MILLIS <= millis()){
					sensor_status		   = SENSOR_ACTIVE;
					sensor_blocked_millis += SENSOR_SENSITIVITY_MILLIS;
				}

			}else if(sensor_status != SENSOR_RESTING){
				sensor_status		  = SENSOR_RESTING;
				sensor_blocked_millis = millis();
			}

			if(last_sensor_status != sensor_status){
				serialPrint("sensor_status: ");
				serialPrintln(sensor_status);
			}
		}

		void bufferDosageLoop()
		{
			// Creamos buffer de activaciones de Sensor
			if(sensor_blocked_millis > update_sensor_blocked_millis){
				update_sensor_blocked_millis = millis();

				// SENSOR_ACTIVE
				if(sensor_status == SENSOR_ACTIVE){
					active_start_time_millis[active_sensor_idx] = millis();
					active_end_time_millis[active_sensor_idx]   = 0;

				// SENSOR_RESTING
				}else if(sensor_status == SENSOR_RESTING && last_sensor_status == SENSOR_ACTIVE){
					active_end_time_millis[active_sensor_idx] = millis();

					active_sensor_idx >= CONVEYOR_BELT_BUFFER_SIZE -1?
						active_sensor_idx = 0:
						active_sensor_idx++;
				}
			}
		}

		void dosageLoop()
		{
			// Actualizamos el estado del dosage_status
			uint8_t original_dosage_status = dosage_status;

			// Inicia DOSAGE_IN_DELAY
			if(active_start_time_millis[active_ssr_idx] != 0 && active_start_time_millis[active_ssr_idx] <= millis()){
				dosage_status = DOSAGE_IN_DELAY;

				// Inicia DOSAGE_DOSING
				if(active_start_time_millis[active_ssr_idx] + (delay_level * DELAY_TIME_STEP_MILLIS) <= millis()){
					dosage_status = DOSAGE_DOSING;

					if((
						// MODO_DISPENSER
						dosage_mode == MODO_DISPENSER &&
						(active_start_time_millis[active_ssr_idx] + (delay_level * DELAY_TIME_STEP_MILLIS) + DOSAGE_TIME_INIT_MILLIS + (dosage_level * DOSAGE_TIME_STEP_MILLIS) <= millis())
					) ||
					(
						// MODO_CONVEYOR_BELT
						dosage_mode == MODO_CONVEYOR_BELT &&
						(
							active_end_time_millis[active_ssr_idx] != 0 &&
							active_end_time_millis[active_ssr_idx] + (delay_level * DELAY_TIME_STEP_MILLIS) <= millis()
						)
					)){
						// de nuevo a DOSAGE_RESTING
						dosage_status = DOSAGE_RESTING;

						active_start_time_millis[active_ssr_idx] = 0;
						active_end_time_millis[active_ssr_idx]	 = 0;

						active_ssr_idx >= CONVEYOR_BELT_BUFFER_SIZE -1?
							active_ssr_idx = 0:
							active_ssr_idx++;
					}
				}
			}


			if(original_dosage_status != dosage_status)
				dosage_last_change_millis = millis();

			// Actualizamos estado del SSR
			dosage_status == DOSAGE_DOSING?
				digitalWrite(SSR_PIN, ENCENDIDO):
				digitalWrite(SSR_PIN, APAGADO);
		}


	/* Debug
	**************************************************/
		void debugSetup()
		{
			#ifdef DEBUG_SERIAL_ENABLE
				serialPrint("in_setup: ");
				serialPrint(in_setup);

				serialPrint("   dosage_mode: ");
				serialPrint(dosage_mode);

				serialPrint("   delay_level: ");
				serialPrint(delay_level);

				serialPrint("   dosage_level: ");
				serialPrint(dosage_level);

				serialPrintln("");
			#endif
		}

		void debugDosage()
		{
			#ifdef DEBUG_SERIAL_ENABLE
				serialPrint("active_sensor_idx: ");
				serialPrint(active_sensor_idx);
				serialPrint("  - active_ssr_idx: ");
				serialPrint(active_ssr_idx);
				serialPrintln("");

				for(int i = 0; i < CONVEYOR_BELT_BUFFER_SIZE; ++i)
				{
					serialPrint("     ");
					serialPrint(i);
					serialPrint(": ");
					serialPrint(active_start_time_millis[i]);
					serialPrint(" - ");
					serialPrint(active_end_time_millis[i]);
				}

				serialPrintln("");
				serialPrintln("");
			#endif
		}


/**************************************************
* SETUP & LOOP
**************************************************/
	void setup()
	{
		deviceInit();
	}

	void loop()
	{
		deviceLoop();
	}
