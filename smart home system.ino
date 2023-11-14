#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define OFF 0x0
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7 



enum State{ // used to define FSM states
    Synchronisation = 0,//0
    MainPhase = 1,//1
    AddDevice = 2,//2
    StateOfSmartDevice,//3
    PowerOfDevice,//4
    RemoveDevice,//5
    ButtonSelect,//6
    ButtonUp,//7
    ButtonDown,//8
    } ;

enum State CurrentState = Synchronisation; // sets the current state of FSM to Synchonisation

struct Device { // creates a structure for all devices e.g input A-HJL-C-Garden
  String DeviceID; //HJL
  char Type; // C
  String Location; // Garden
  bool State = false; // off == false, on = true
  int Power;
};

Device Devices[15]; // creates array of 20 devices 
int current_device_index = 0; // global var used to track current index of Devices array

Device DevicesOrdered[15]; //array of 20 devices (used as an ordered array of Devices)
int num_ordered_devices = 0; // global variable for number of ordered devices ( 1 + current_device_index)

int lcd_current_index = 0; // a global var for tracking curret index on the lcd screen
String UserInput =""; // global variable where user inputs are stored 

byte upArrow[] = { // bitmap up arrow for UDCHARS extensions
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00000,
  0b00000
};

byte downArrow[] = { // bitmap down arrow for UDCHARS extensions
  0b00000,
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};

byte degreeSymbol[8] = { // bitmap for a degree symbol
  0b00110,
  0b01001,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

#define BUTTON_UP 0x08     // defining lcd buttons 
#define BUTTON_DOWN 0x04   
#define BUTTON_LEFT 0x10   
#define BUTTON_RIGHT 0x02  
#define BUTTON_SELECT 0x01

//TODO ,,, buttonselect, , Extensions
int compare_two_events(const void* a, const void* b){  // a comparison function used to order devices
  Device* ptrA = (Device*)a;
  Device* ptrB = (Device*)b;
  if (ptrA->DeviceID < ptrB->DeviceID) {
    return -1;
  } 
  else if (ptrA->DeviceID > ptrB->DeviceID) {
    return 1;
  } 
  else {
    return 0;
  }
}

void OrderDevices(){
  qsort(DevicesOrdered, num_ordered_devices, sizeof(Device), compare_two_events); // sorts  DevicesOrdered array using the comparison function above
  /////
  //Serial.println("Ordering devices");
  /////
  /////for(int i=0; i < num_ordered_devices; i++){
    /////
    //Serial.println(DevicesOrdered[i].DeviceID);
    /////
  /////}
  
}

void UpdateDisplay(int i){ // a function which takes an int as its parameter and displays the info of that index in OderedDevices to the lcd
  //first row
  lcd.clear();
  if(i >0){
    lcd.write(byte(0));//up arrow if not first element in list
  }
  else{
     lcd.setCursor(1, 0);
  }
  lcd.print(DevicesOrdered[i].DeviceID); // display device id on lcd
  lcd.setCursor(5, 0);
  lcd.print(DevicesOrdered[i].Location);// display device location on lcd

  //second row
  lcd.setCursor(0, 1);
  if(i <num_ordered_devices-1 ){//down arrow if not last element in list
    lcd.write(byte(1));//down arrow
  }
  else{
    lcd.setCursor(1, 1);
  }
  lcd.print(DevicesOrdered[i].Type);
  lcd.setCursor(3, 1);
  if (DevicesOrdered[i].State ==true){ // changes backlight based on state of device
    lcd.print(" ON");
    lcd.setBacklight(GREEN);
    
  }
  else{
    lcd.print("OFF");
    lcd.setBacklight(YELLOW);
 }
  lcd.setCursor(7, 1);
  if ((DevicesOrdered[i].Type == 'S') ||(DevicesOrdered[i].Type == 'L')){
    lcd.print(DevicesOrdered[i].Power);
    lcd.print("%");
  }
  if (DevicesOrdered[i].Type == 'T'){
     lcd.print(DevicesOrdered[i].Power);
     lcd.write(byte(2));
     lcd.print("C");
  }
  
}

void Add_Device(String UserInput){
  /////
  //Serial.println(UserInput);//Debug checks input
  /////
  bool AlreadyAdded = false;
  for (int i =0; i<=current_device_index; i++){// used to check if device is already added
    String tempId=UserInput.substring(2,5);
    if (tempId == Devices[i].DeviceID){ // if devices is already added 
      Devices[i].Type =  UserInput[6]; //update device type
      Devices[i].Location = UserInput.substring(8); // update device location
      AlreadyAdded = true;
      //Serial.println("already added");
      //break;
    }
  }
  String tempId; // creates temp variables
  char tempType;
  String tempLocation;
  

  tempId=UserInput.substring(2,5); //gets Id of device
  char c = UserInput[6]; // gets type of device
  tempType=c; // gets type of device
  tempLocation=UserInput.substring(8); // gets device location

  
  bool valid_char = false;
  bool valid_format = false;
  bool valid_location = false;

  if((strcmp(UserInput.substring(1,2).c_str(), "-") == 0) & (strcmp(UserInput.substring(5,6).c_str(), "-") == 0) & (strcmp(UserInput.substring(7,8).c_str(), "-") == 0)){
    //Serial.println("heloo");
    valid_format = true;
  }
  else{
    //Serial.println("invalid format");
  }


  if (c == 'S' || c == 'L' || c == 'O' || c == 'T' || c == 'C'){
    valid_char = true;
  }
  else{
    //Serial.println("invalid chars");
  }

  if(tempLocation.length() > 1){
    valid_location = true;
  }
  else{
    //Serial.println("doesnt have a location");
  }

  
  if ((AlreadyAdded == false) && (valid_format == true) && (valid_char == true) && (valid_location == true)){
    //Serial.println("running");
    Devices[current_device_index].DeviceID = tempId;//assign temp var to a new device in Devices
    Devices[current_device_index].Type = tempType;
    Devices[current_device_index].Location = tempLocation;

    DevicesOrdered[current_device_index].DeviceID = tempId; //assign temp var to new device in DevicesOrdered
    DevicesOrdered[current_device_index].Type = tempType;
    DevicesOrdered[current_device_index].Location = tempLocation;
    /////
    //Serial.println(Devices[current_device_index].DeviceID);
    /////

    // updates indexes of global variables
    current_device_index++;
    num_ordered_devices++;
    // calls function to order devcies
    OrderDevices();
    //Serial.println("finish");
    
  }
  else{
    if (lcd_current_index >=1){
      OrderDevices();
    }
   
  }
}

void Set_State(String UserInput){
  int index_of_device = 0;
  //bool found = false;
  for (int i =0; i<=current_device_index; i++){// used to get index of device id in devices arr
    String tempId=UserInput.substring(2,5);
    if (tempId == Devices[i].DeviceID){
      String tempState = UserInput.substring(6);
      if (tempState == "ON"){
        Devices[i].State =true;

      }
      else{
        Devices[i].State =false;
      }
      //found = true;
      //Serial.println(Devices[i].State);
    }
  }
  //break;// exits loop
      // Serial.println(Devices[i].State);
  
  for(int i=0; i < num_ordered_devices; i++){// loop copies contents of devices arr to orderedDevices arr
    DevicesOrdered[i] = Devices[i];
  }
  OrderDevices(); // sorts orderedDevices arr
  UpdateDisplay(lcd_current_index);
}

void Set_Power(String UserInput){
  for (int i =0; i<=current_device_index; i++){// used to get index of device id in devices arr
    String tempId=UserInput.substring(2,5); 
    bool valid_format = false;
    if (tempId == Devices[i].DeviceID){//checks if device ID at current index equals user input
      bool valid_format = false;
      if((strcmp(UserInput.substring(1,2).c_str(), "-") == 0) & (strcmp(UserInput.substring(5,6).c_str(), "-") == 0)){
        //Serial.println("heloo");
        valid_format = true;
      }
      if (valid_format == true){
        if ((Devices[i].Type == 'O') || (Devices[i].Type == 'C')){// if Type of device is outlet or camera dont change any values
          Serial.println("device not supported");
          //break;
        }
      
        else{// if device does support set power
          String tPower = UserInput.substring(6);
          int tempPower =  tPower.toInt();// changes userinput to int
          if ((Devices[i].Type == 'S') || (Devices[i].Type == 'L')){
            if ((tempPower >=0) & (tempPower <=100)){ // checks within range
              Devices[i].Power =tempPower;
              /////
              //Serial.println(Devices[i].Power);
              //break;
              /////
            }
            else{
              Serial.println("Out of range (0,100)");
              //break;
            }
          }
          if (Devices[i].Type == 'T'){
            if ((tempPower >=9) & (tempPower <=32)){// checks if within range
              Devices[i].Power =tempPower;
              //break;
            
            }
            else{
              Serial.println("Out of range (9,32) ");
              //break;
            }
          }
        }
      
        //break;
      }
      else{
        //Serial.println("invalid format");
      }
    } 
  }
  for(int i=0; i <= num_ordered_devices; i++){// loop copies contents of devices arr to orderedDevices arr
    DevicesOrdered[i] = Devices[i];
  }
  OrderDevices(); // sorts orderedDevices arr
  UpdateDisplay(lcd_current_index);
}

/// comment out serial.prints
void Remove_Device(String UserInput){ 
  String tempId=UserInput.substring(2,5);
  int new_index = 0;

  for (int i =0; i<=num_ordered_devices; i++){//once it hits the index of the device to remove, it decrements the index of all elements above it
    if (tempId == Devices[i].DeviceID){
      /////
      //Serial.println("removing");
      //Serial.println(Devices[i].DeviceID);
      //Serial.println("------");
      /////
    }
    else{
      Devices[new_index] = Devices[i];
      new_index++;
    }
  } 
  
  current_device_index --;// updating pointers as now have one less device
  num_ordered_devices--;

  for(int i=0; i < num_ordered_devices; i++){// loop copies contents of devices arr to orderedDevices arr
    DevicesOrdered[i] = Devices[i];
  }

  OrderDevices(); // sorts orderedDevices arr
  for(int i=0; i < num_ordered_devices; i++){
    //Serial.println(DevicesOrdered[i].DeviceID);
  }
  UpdateDisplay(lcd_current_index);
}


// testing A-PZW-S-LivingRoom  A-BDA-T-Entrance A-HJL-C-Garden

int freeRam () {
  extern int __heap_start, *__brkval; 
  int n; // creates var
  return (int) &n - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  // above line returns free memory in bytes minusing the new var n created from start or end of heap.

}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  /// creates the special symbols (bitmaps)
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);
  lcd.createChar(2, degreeSymbol);

  lcd.setBacklight(VIOLET);
}


void loop() {
// testing A-PZW-S-LivingRoom  A-BDA-T-Entrance A-HJL-C-Garden A-MHJ-S-Home S-BDA-ON P-BDA-12 S-PZW-ON 
  switch (CurrentState){
    case Synchronisation://Synchronisation 
        Serial.print("Q");// prints Q to serial monitor every second untill it recives an "X"
        if (Serial.available() > 0){
          String input = Serial.readString();
          if (input == "X"){// once it gets the X it sets backlight to white, prints basic and sets current state to mainphase
            delay(1000);
            CurrentState = MainPhase;  
            Serial.println("BASIC");
            Serial.println("");
            lcd.setBacklight(WHITE);     
          }
        }
        else{
           delay(1000);
        }
        break;
     
        
    case MainPhase:// main phase  
        //Serial.println("main phase");
        //Serial.println(freeRam());
        //delay(500);
        
        if (Serial.available() > 0){// checks if theres been an input, else checks if button has been pressed
          UserInput = Serial.readString();
         // Serial.println(UserInput);
          if(strcmp(UserInput.substring(0,1).c_str(), "A") == 0){ //checks first letter of input if A it sets current state to add device
         //   Serial.println("entered case A");
            CurrentState = AddDevice;
          //  Serial.println("finish");
          }
          if((strcmp(UserInput.substring(0,1).c_str(), "S") == 0)& (current_device_index !=0)){ // depending on first letter of user input it changes the currentstate, (as long as a device has already been added)
          //  delay(1000);
            CurrentState = StateOfSmartDevice;
            //
             
          }
          if((strcmp(UserInput.substring(0,1).c_str(), "P") == 0)& (current_device_index !=0)){ //
         //   delay(1000);
            CurrentState = PowerOfDevice;
          }

          if((strcmp(UserInput.substring(0,1).c_str(), "R") == 0)& (current_device_index !=0)){ 
         //   delay(1000);
            //Serial.println("entered case R");
            CurrentState = RemoveDevice;
          }
    
        }
        else{
            //delay(1000);
            uint8_t pressedButtons = lcd.readButtons(); 
            // first version of handling select button
            /*
            if(pressedButtons & BUTTON_SELECT){ 
              delay(1000);
              if(pressedButtons && BUTTON_SELECT){// if select button is held for longer than a second sets state to btn Select
                Serial.println("5");
                CurrentState = ButtonSelect;
              }

            }
            */
            if(pressedButtons & BUTTON_SELECT){
              CurrentState = ButtonSelect;
            }

            //lcd displays devices in alphabetical order
            //lcd default is first element

            //below code deals with naviation of lcd screen, uses current lcd index and the number of devices to determine what button presses are allowed
            // e.g lcd default first element therefore can only go down on first command (excluding select) 
    
            if ((lcd_current_index > 0)&(lcd_current_index < (num_ordered_devices-1))){
              if (pressedButtons & BUTTON_DOWN) {  // check if the down button is pressed
                //Serial.println("1");
                CurrentState = ButtonUp;
              }
              
              if (pressedButtons & BUTTON_UP) {  // check if the up button is pressed
               // Serial.println("2");
                CurrentState = ButtonDown;
              }
            }
            else{
              if (lcd_current_index == 0){ // if at first element in the list
                if (pressedButtons & BUTTON_DOWN) {  // check if the down button is pressed
                  //Serial.println("3");
                  CurrentState = ButtonUp;
                  //Serial.println("3.5");
                }
              }
              if (lcd_current_index==(num_ordered_devices-1)){// if at the last element in the list
                if (pressedButtons & BUTTON_UP) {  // check if the up button is pressed
                  //Serial.println("4");
                  CurrentState = ButtonDown;
                }
              }
            }

        }
     break;

    case AddDevice:// AddDevice e.g A-HJL-C-Garden
        //Serial.println("add a device");
        //Serial.println(UserInput);
        Add_Device(UserInput);
        UpdateDisplay(0);
       // delay(1000);
        CurrentState = MainPhase;
      break;

    case StateOfSmartDevice:
       // Serial.println("StateOfSmartDevice");
        Set_State(UserInput);
        //delay(1000);
        CurrentState = MainPhase;
    
      break;

    case PowerOfDevice:
        //Serial.println("PowerOfDevice");
        Set_Power(UserInput);
        //delay(1000);
        CurrentState = MainPhase;
     
      break;

    case RemoveDevice:
        //Serial.println("RemoveDevice");
        Remove_Device(UserInput);
        delay(500);
        CurrentState = MainPhase;
      break;
      
    case ButtonSelect: 
       // Serial.println("Select");
        lcd.clear();
        lcd.setBacklight(VIOLET);
        lcd.print("F220759");
        lcd.setCursor(0, 1);
        lcd.print(freeRam());
        //Serial.println(freeRam());
        delay(1000);
        CurrentState = MainPhase;
        UpdateDisplay(lcd_current_index);
      break;

    case ButtonUp:
        //delay(500);
        //Serial.println("Down");
        lcd.clear();
        lcd_current_index ++;
        UpdateDisplay(lcd_current_index);
        CurrentState = MainPhase;
      break;

    case ButtonDown:
       // delay(500);
        //Serial.println("Up");
        lcd.clear();
        lcd_current_index --;
        UpdateDisplay(lcd_current_index);
        CurrentState = MainPhase;
      break;
  
  }

}

