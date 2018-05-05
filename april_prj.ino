/*
 * Woojeong Park
 * 04/30/2018
 * SNU Interactive Media
 */
#include <CapacitiveSensor.h>
#include <Stepper.h>

int const green_led_root = 9;
int const IR_sensor = A0;//infrared sensor
CapacitiveSensor cps = CapacitiveSensor(3,2);        
// 1M resistor between pins 3 & 2, pin 2 is sensor pin, add a wire and or foil if desired

const int neckRevolution = 200;  // change this to fit the number of steps per revolution
const int spineRevolution = 180;
const int spineRevolution2 = 100;

// Capacitive Sensor Detection
bool back_touched = false; 

bool is_near = false;
bool is_far = false;
const int NEAR_DISTANCE = 35;
const int FAR_DISTANCE = 70;


/* Light Strength */
const int MEDIUM_LIGHT = 50;
const int ARE_YOU_NEAR_LIGHT = 10;
const int DIMMEST_LIGHT = 5;


/* Distance Detectable */
const int DIST_LOWEST = 5;
const int DIST_HIGHEST = 90;


static const unsigned long REFRESH_INTERVAL = 5000; // 1 sec

// initialize the stepper library on pins 8 through 11:
Stepper yaw_stepper(neckRevolution, 4, 5, 6, 7);
Stepper pitch_stepper(spineRevolution, 13, 12, 11, 8);

int spine_up_level = 0;
int head_turned_level = 0;

/* Check if there's no one near the doll. */
unsigned long timer = 0;
unsigned long last_timer = 0;

/* Wait and see if the user stays near for at least 3 secs. */
unsigned long start_near = 0;
unsigned long current_near = 0;

/* Wait and see if the user stays far for at least 3 secs. */
unsigned long start_far = 0;
unsigned long current_far = 0;
int far_check[8] = {0};

/* Will not use this, just for test. Automatically undo the recent yaw-rotation. */
unsigned long time_activated;
unsigned long current_time;

unsigned long cap_start_time = 0;
unsigned long cap_last_time = 0;
unsigned long avg_cap_start_time = 0;
unsigned long avg_cap_last_time = 0;
const int CAP_STEP_INTERVAL = 100;


long capacitance;//neck revolution
long touched_capacitance;//second spine revolution
long cap_for_normal;//used only when adding steps to calculate normal_capacitance
long normal_capacitance;
const int CAP_ARR_LEN = 80;
long cap_arr[CAP_ARR_LEN] = {0};
int cap_ind = 0;
long avg_cap = 0;
const int AVG_CAP_ARR_LEN = 30;
long avg_cap_arr[AVG_CAP_ARR_LEN] = {0};
int avg_cap_ind = 0;
long FIRST_CAP_GAP = 250;
long SECOND_CAP_GAP = 20;
bool cap_arr_initialized = false;


int green_strength = 0;
int brightest = -1;


void setup() {
  // set the speed at 15 rpm:
  yaw_stepper.setSpeed(30);
  pitch_stepper.setSpeed(30);
  cps.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   
  
  // initialize the serial port:
  //pinMode(TEST_SWITCH, INPUT);
  pinMode (IR_sensor, INPUT);
  pinMode (green_led_root, OUTPUT);

  start_far = 0;
  current_far = 0;
  
  Serial.begin(9600);
}

void loop() {
  /* Detect the distance between the doll and the user. */
  long IR_value = analogRead(IR_sensor);
  long IR_range = gp2y0a21yk(IR_value);

  //Serial.println (IR_value);
  Serial.print (IR_range);
  Serial.println (" cm");
  //Serial.println ();
 
  
  





  /* Spine Rotation */  

  if(start_near == 0){
    if(IR_range <= NEAR_DISTANCE){
      start_near = millis();
    }
  }
  else{
    current_near = millis();

    switch( (current_near - start_near) / 1000){
      /*
      case 4:
        Serial.println("====== 4 secs past");
        if(IR_range <= NEAR_DISTANCE){//The user is still near!
          is_near = true;  
        }
        break;
      */
      case 3:
        Serial.println("====== 3 secs past");
        if(IR_range <= NEAR_DISTANCE){//The user is still near!
          is_near = true;  
        }
        break;
      case 2:
        Serial.println("====== 2 secs past");
        break;
      case 1:
        Serial.println("====== 1 sec past");
        break;
      case 0:
        break;
      default:
        flush_near_time();    
    }

  
    
    if(spine_up_level == 0){
      /* Green Light Control */
        if(current_near - start_near >= 500) {
          if(IR_range <= NEAR_DISTANCE){
            if(IR_range <= 30){
              green_strength = map(IR_range, DIST_LOWEST, 30, 250, 180);
            }
            else{
              green_strength = map(IR_range, 30, NEAR_DISTANCE, 180, MEDIUM_LIGHT);
            }
             
            if (is_near){//remember this brightness
              brightest = green_strength;
            }
          }
        }
      } 
  }//else statement
  

  if(spine_up_level == 0){
    if(IR_range > NEAR_DISTANCE){
      current_near = 0;
      start_near = 0; 

      if(IR_range <= FAR_DISTANCE){
        green_strength = map(IR_range, NEAR_DISTANCE, FAR_DISTANCE, MEDIUM_LIGHT, ARE_YOU_NEAR_LIGHT);  
      }
    }
  }





  analogWrite(green_led_root, green_strength);




  /* Check distance every second. 
   * So that we can refresh this whole cycle and flags etc
   * if the user is far away for over 8 secs.
   */


  if(start_far == 0){
    if (IR_range > NEAR_DISTANCE){
      start_far = millis();  
    }
  }
  else{
    current_far = millis();
    cap_start_time = millis();
    switch( (current_far - start_far) / 1000 ){
      case 8:
        far_check[7] = (IR_range > NEAR_DISTANCE) ? 1 : 0;
        if (far_check[7] * far_check[6] * far_check[5] * far_check[4] * far_check[3] * far_check[2] * far_check[1] * far_check[0] == 1){
          //was far for 8 secs
          is_far = true;
          
          initializeAll();
        }
        else{        
          flush_far_check();  
        }
        break;
      case 7:
        Serial.println(">>>>> 7 secs past");
        if(far_check[6] == 0){
          far_check[6] = (IR_range > NEAR_DISTANCE) ? 1 : 2;
        }
        if ((far_check[6] * far_check[5]) == 4){//if someone was near for consecutive 2 secs, flush the counter immediately.
          flush_far_check();
        }
        
        break;
      case 6:
        Serial.println(">>>>> 6 secs past");
        if(far_check[5] == 0){
          far_check[5] = (IR_range > NEAR_DISTANCE) ? 1 : 2;
        }
        
        if ((far_check[5] * far_check[4]) == 4){
          flush_far_check();
        }
        break;
      case 5:
        Serial.println(">>>>> 5 secs past");
        if(far_check[4] == 0){
          far_check[4] = (IR_range > NEAR_DISTANCE) ? 1 : 2;
        }
        
        if ((far_check[4] * far_check[3]) == 4){
          flush_far_check();
        }
        break;
      case 4:
        Serial.println(">>>>> 4 secs past");
        if(far_check[3] == 0){
          far_check[3] = (IR_range > NEAR_DISTANCE) ? 1 : 2;
        }
        
        if ((far_check[3] * far_check[2]) == 4){
          flush_far_check();
        }
        break;
      case 3:
        Serial.println(">>>>> 3 secs past");
        if(far_check[2] == 0){
          far_check[2] = (IR_range > NEAR_DISTANCE) ? 1 : 2;
        }
        
        if ((far_check[2] * far_check[1]) == 4){
          flush_far_check();
        }
        break;
      case 2:
        Serial.println(">>>>> 2 secs past");
        if(far_check[1] == 0){
          far_check[1] = (IR_range > NEAR_DISTANCE) ? 1 : 2;
        }
        
        if ((far_check[1] * far_check[0]) == 4){
          flush_far_check();
        }
        break;
      case 1:
        Serial.println(">>>>> 1 sec past");
        if(far_check[0] == 0){
          far_check[0] = (IR_range > NEAR_DISTANCE) ? 1 : 2;
        }
        
        break;
      case 0:
        break;
      default: //??
        flush_far_check();
    }


    // check normal capacitance while being far
    if(cap_last_time == 0){
      cap_last_time = cap_start_time;
    }
    
    
    
    if (cap_start_time - cap_last_time >= CAP_STEP_INTERVAL){
      cap_last_time = millis();
      
      cap_for_normal =  cps.capacitiveSensor(30);
      cap_arr[cap_ind] = cap_for_normal;


      long cap_sum = 0;
      if(cap_arr[CAP_ARR_LEN-1] == 0){
        for(int i = 0; i <= cap_ind; i++){
          cap_sum += cap_arr[i];
        }
        normal_capacitance = cap_sum / (cap_ind+1);
      }
      else{        
        for(int i = 0; i < CAP_ARR_LEN; i++){
          cap_sum += cap_arr[i];
        }
        normal_capacitance = cap_sum / CAP_ARR_LEN;
      }
      

      if(cap_ind < CAP_ARR_LEN - 1){
        cap_ind += 1;
      }
      else{
        cap_ind = 0;  
      }
            
      
    }//capacitance step interval 
  }



  Serial.print("normal capacitance: ");
  Serial.println(normal_capacitance);
  
  Serial.print("capacitance: ");
  Serial.println(cap_for_normal);



 
  if(spine_up_level == 0 && is_near){
    Serial.println("Hello, stranger!");
    pitch_stepper.step(spineRevolution);
    spine_up_level = 1;
  }




  /* Head Turning */
  if (spine_up_level > 0){
    
    green_strength = brightest;
    

    capacitance =  cps.capacitiveSensor(30);
           
    
    if(head_turned_level == 0){
      if (cap_arr[CAP_ARR_LEN - 1] != 0){
        FIRST_CAP_GAP = (normal_capacitance > 1000)? 80 : 50;
        //FIRST_CAP_GAP = (normal_capacitance > 1000)? 150 : 30;//using battery
        if (capacitance - normal_capacitance >= FIRST_CAP_GAP){
          back_touched = true;
          touched_capacitance = capacitance;
        }
      }
 
      if(back_touched){
        
        Serial.println("Hey..");
        
    Serial.print("CAPACITANCE: ");
    Serial.println(capacitance);
        yaw_stepper.step(neckRevolution);
        head_turned_level = 1;
        time_activated = millis();  
      }
    }




    if(head_turned_level == 1){  
      avg_cap_start_time = millis();
      
      if(avg_cap_last_time == 0){
        avg_cap_last_time = avg_cap_start_time;
      }
      
      
      
      if (avg_cap_start_time - avg_cap_last_time >= CAP_STEP_INTERVAL){
        avg_cap_last_time = millis();      
        //capacitance =  cps.capacitiveSensor(30);
        avg_cap_arr[avg_cap_ind] = capacitance;
          
          
        if(avg_cap_arr[AVG_CAP_ARR_LEN - 1] != 0){//if array full
          long avg_cap_sum = 0;
          for (int i = 0; i < AVG_CAP_ARR_LEN; i++){
            avg_cap_sum += avg_cap_arr[i];  
          }
          avg_cap = avg_cap_sum / AVG_CAP_ARR_LEN;
        }

        
        if(avg_cap_ind == AVG_CAP_ARR_LEN - 1){
          avg_cap_ind = 0;
        }
        else{
          avg_cap_ind += 1;  
        }
        
      }//average capacitance
    }//head_turned_level == 1 
    
  
    if(head_turned_level == 1){
      current_time = millis();
      if(spine_up_level == 1){
        //if(avg_cap - touched_capacitance >= SECOND_CAP_GAP){
        SECOND_CAP_GAP = 50; //if not battery 
        if(avg_cap - normal_capacitance >= SECOND_CAP_GAP){
          pitch_stepper.step(spineRevolution2);
          spine_up_level = 2;
        }
      }

      
    
      if(!back_touched){
        Serial.println("Turning away");
        yaw_stepper.step(-neckRevolution);
        head_turned_level = 2;
      }
    }
    
    if(head_turned_level == 2){
      Serial.println("PROBLEMATIC PHASE");
      if(IR_range > NEAR_DISTANCE){
        Serial.println("You're leaving..");
        pitch_stepper.step(-spineRevolution);
        spine_up_level = 0;
        initializeAll();
      }  
    }     
  }//spine_up_level == 1 or 2
  
  last_timer = millis();
  delay(10);
}


long gp2y0a21yk(long IR_value){
  if (IR_value < 10) IR_value = 10;
  return ((6787.0 / (IR_value - 3.0)) - 4.0);
}


void initializeAll(){
  Serial.println("Initializing All!");

  if(spine_up_level != 0 && head_turned_level != 0){
    for(int i = 0; i < CAP_ARR_LEN; i++){
      cap_arr[i] = 0;
      cap_arr_initialized = true;
    }
  }
  else{
    cap_arr_initialized = false;
  }


  if(head_turned_level == 1){
    yaw_stepper.step(-neckRevolution);
  }
  if(spine_up_level == 1){
    pitch_stepper.step(-spineRevolution);
  }
  else if (spine_up_level == 2){
    pitch_stepper.step(-spineRevolution-spineRevolution2);
  }
  
  last_timer = 0;
  timer = 0;
  
  is_far = false;

  flush_near_time();

  
  flush_far_check();
  
  spine_up_level = 0;
  head_turned_level = 0;
  back_touched = false;
  time_activated = 0;
  current_time = 0;
  green_strength = 0;  
  cap_start_time = 0;
  brightest = 0;
  
}

void flush_far_check(){
  start_far = 0;
  current_far = 0;
  for(int i = 0; i < 8; i++){
    far_check[i] = 0;
  }
}

void flush_near_time(){
  start_near = 0;
  current_near = 0;
  is_near = false;  
}
