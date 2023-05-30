#include <M5Core2.h>
#include <string.h>
#include <math.h>

//variables below are for tanks jpg images needed for M5.Lcd.drawJpg()  
#define IMG Tank // import from Tank.c file
#define IMGB TankB
#define picArray extern unsigned char
picArray IMG[];
picArray IMGB[];
size_t IMGSize = 6514;
size_t IMGBSize = 7777;

double G = 9.81; 
double EPSILON = 0.000001;

//variables below are used to determine set value 
bool isReset = true;
bool isSetMode = false; 
bool isSetCascadeType = false;
bool isSetTankAHeight = false;
bool isSetTankAWidth = false;
bool isSetTankALength = false;
bool isSetTankAWaterHeight = false;
bool isSetTankAWaterSupply = false;
bool isSetTankAWaterDrainSurface = false;
bool isSetTankBHeight = false;
bool isSetTankBWidth = false;
bool isSetTankBLength = false;
bool isSetTankBWaterHeight = false;
bool isSetTankBWaterSupply = false;
bool isSetTankBWaterDrainSurface = false;
bool isSetTankCHeight = false;
bool isSetTankCWidth = false;
bool isSetTankCLength = false;
bool isSetTankCWaterHeight = false;
bool isSetTankCWaterSupply = false;
bool isSetTankCWaterDrainSurface = false;
String currentSet = "Wybierz tryb pracy";
String currentSetTank = "zbiornik A";

//variables below are true when object need to drown on display
bool drawBtns = true;
bool drawTank = true;
bool drawRect = true;

//variables for time mesurements
unsigned long startTaskTime = 0;
unsigned long displayWaterLevelTime = millis();
unsigned long calculateWaterFlowTime = millis();
unsigned long calculateWaterFlowTime2 = millis();
unsigned long CarryOnPresetTaskTime = millis();
int alarmCount = 0;

int currentProperty = 1; //1-> wysokosc, 2->szerokosc, 3-> dlugosc, 4-> wysokosc wody, 5->doplyw wody, 6->powierzchnia odplywu, 7-> ilosc wody, 8-> odplyw wody  
int cascadeType; // 1- A->B->C ; 2 - A->C B->C 
const char* propertyNames[8]={"wysokosc", "szerokosc", "dlugosc", "wysokosc wody", "doplyw", "powierzchnia", "ilosc wody", "odplyw"};
int page = 1; // 1->mainscreen; 2->detailscreen
int mode = 1; // 1 - carries out task from attached file "wyklad_USCtankC.valve1In_.pdf"; 2- carries out task from attached file "Tryb pracy 2.pdf"; 3 - user sets all parameters by himself/herself   

//struct where all information about tank can be stored
typedef struct {
  double height;//m
  double width;//m
  double length;//m
  double waterHeight;//m
  double waterSupply; //m3/s
  double waterSupplyValve1In; //m3/s 
  double waterSupplyValve2In; //m3/s 
  double waterDrainSurface; // m2 
  double waterDrain; //m3/s  
  double waterQuantity; //m3  
  bool alarm;
  double previousWaterSupply; //m3/s
  double previousWaterDrainSurface; // m2 
  //variables for preset operating mode   
  int state; 
  int timeA;
  int timeB;
  int valve1In; // 0 - off; 1 - on; something different - not used
  int valve2In;
  int valveOut;
  int heightLow; // 0 -if water height is below; 1 - if water height is above
  int heightMiddle;
  int heightHigh;             
}tank;

//init tanks with defoult values
tank tankA = {.height = 1.0, .width = 1.0, .length = 1.0, .waterHeight= 0.5, .waterSupply = 0.03, .waterDrainSurface = 0.01, .alarm = false, .state = 1, .valve1In = 1, .valve2In = 0, .valveOut = 1, .heightLow = 0, .heightMiddle = 0, .heightHigh = 0};
tank tankB = {.height = 1.0, .width = 1.0, .length = 1.0, .waterHeight= 0.5, .waterSupply = 0.03, .waterDrainSurface = 0.01, .alarm = false, .state = 1, .valve1In = 1, .valve2In = 0, .valveOut = 1, .heightLow = 0, .heightMiddle = 0, .heightHigh = 0};
tank tankC = {.height = 1.0, .width = 1.0, .length = 1.0, .waterHeight= 0.5, .waterSupply = 0.03, .waterDrainSurface = 0.01, .alarm = false, .state = 1, .valve1In = 1, .valve2In = 1, .valveOut = 1, .heightLow = 0, .heightMiddle = 0, .heightHigh = 0};

// Defines the buttons. Colors in format {bg, text, outline}
ButtonColors on_clrs = {LIGHTGREY, WHITE, WHITE};
ButtonColors off_clrs = {BLACK, WHITE, WHITE};
ButtonColors on_clrs_reset = {LIGHTGREY, WHITE, WHITE};
ButtonColors off_clrs_reset = {RED, WHITE, WHITE};
ButtonColors on_clrs_detail = {LIGHTGREY, LIGHTGREY, LIGHTGREY};
ButtonColors off_clrs_detail = {LIGHTGREY, LIGHTGREY, LIGHTGREY};

// variables for buttons x, y, width and height will be set just before button is drown.
Button cascadeTypeBtn1(0, 0, 0, 0, false ,"A->B->C", off_clrs, on_clrs);
Button cascadeTypeBtn2(0, 0 , 0, 0, false, "A->C B->C", off_clrs, on_clrs);
Button plusBtn(0, 0, 0,0, false ,"+", off_clrs, on_clrs);
Button minusBtn(0, 0, 0,0, false ,"-", off_clrs, on_clrs);
Button plus2Btn(0, 0, 0,0, false ,"+", off_clrs, on_clrs);
Button minus2Btn(0, 0, 0,0, false ,"-", off_clrs, on_clrs);
Button nextBtn(0, 0, 0, 0, false,"Zatwierdz", off_clrs, on_clrs);
Button goBackBtn(0, 0, 0, 0, false,"Powrot", off_clrs, on_clrs);
Button upBtn(0, 0, 0, 0, false, "Up", off_clrs, on_clrs);
Button downBtn(0, 0, 0, 0, false, "Down", off_clrs, on_clrs);
Button resetBtn(0, 0, 0, 0, false, "RESET", off_clrs_reset, on_clrs_reset);
Button detailsBtn(0, 0, 0, 0, false, "", off_clrs_detail, on_clrs_detail);

//function which set choosen by user cascade type 
void setCascadeType(Event& e) {
  // Just so we can type "b." instead of "e.button->"
  Button& b = *e.button;
  M5.Lcd.setCursor(20,40);
  char* n = b.getName();
  cascadeType = (!strcmp(n,"A->B->C" )) ? 1 : (!strcmp(n,"A->C B->C" )) ? 2 : 0;  
  if(cascadeType) {
    isSetCascadeType = true;
    cascadeTypeBtn1.set(0,0,0,0);
    cascadeTypeBtn2.set(0,0,0,0);  
    cascadeTypeBtn1.setLabel("");
    cascadeTypeBtn2.setLabel("");      
    drawBtns = true;
    currentSet = "wysokosc";
  }
  M5.Lcd.clearDisplay();
}

//function to check if two double values are same. sometimes when they should be "==" there was some differece always smaller than EPSILON = 0.000001  
bool AreSame(double a, double b){
    return fabs(a - b) < EPSILON;
}

//function to add one to current set value if user press "+" button 
void pressPlusBtn(Event& e ){
  Button& b = *e.button;
  //draw rect where value is on screen to aviod overlap 
  M5.Lcd.fillRoundRect(100, 80, 120, 80,15, LIGHTGREY) ;
  if (!isSetMode) { mode +=1; if(mode % 4 == 0) mode = 1; return;} 
  if (!isSetTankAHeight) {tankA.height = (tankA.height == 9) ? 9 : tankA.height + 1; return;}
  if (!isSetTankAWidth) {tankA.width = (tankA.width == 9) ? 9 : tankA.width + 1; return;}
  if (!isSetTankALength) {tankA.length = (tankA.length == 9) ? 9 : tankA.length + 1; return;}
  if (!isSetTankAWaterHeight){tankA.waterHeight = (AreSame(tankA.waterHeight,tankA.height)) ? tankA.waterHeight : tankA.waterHeight + 0.1; return;}
  if (!isSetTankAWaterSupply){tankA.waterSupply = (AreSame(tankA.waterSupply, 0.1)) ? 0.1 : tankA.waterSupply + 0.01; return;};
  if (!isSetTankAWaterDrainSurface){tankA.waterDrainSurface = (AreSame(tankA.waterDrainSurface, 0.2)) ? 0.2 : tankA.waterDrainSurface + 0.01; return;}  
  if (!isSetTankBHeight) {tankB.height = (tankB.height == 9) ? 9 : tankB.height + 1; return;}
  if (!isSetTankBWidth) {tankB.width = (tankB.width == 9) ? 9 : tankB.width + 1; return;}
  if (!isSetTankBLength) {tankB.length = (tankB.length == 9) ? 9 : tankB.length + 1; return;}
  if (!isSetTankBWaterHeight){tankB.waterHeight = (AreSame(tankB.waterHeight, tankB.height)) ? tankB.waterHeight : tankB.waterHeight + 0.1; return;}
  if (!isSetTankBWaterSupply){tankB.waterSupply = (AreSame(tankB.waterSupply, 0.1)) ? 0.1 : tankB.waterSupply + 0.01; return;};
  if (!isSetTankBWaterDrainSurface){tankB.waterDrainSurface = (AreSame(tankB.waterDrainSurface, 0.2)) ? 0.2 : tankB.waterDrainSurface + 0.01; return;}  
  if (!isSetTankCHeight) {tankC.height = (tankC.height == 9) ? 9 : tankC.height + 1; return;}
  if (!isSetTankCWidth) {tankC.width = (tankC.width == 9) ? 9 : tankC.width + 1; return;}
  if (!isSetTankCLength) {tankC.length = (tankC.length == 9) ? 9 : tankC.length + 1; return;}
  if (!isSetTankCWaterHeight){tankC.waterHeight = (AreSame(tankC.waterHeight, tankC.height)) ? tankC.waterHeight : tankC.waterHeight + 0.1; return;}
  if (!isSetTankCWaterSupply){tankC.waterSupply = (AreSame(tankC.waterSupply, 0.1)) ? 0.1 : tankC.waterSupply + 0.01; return;};
  if (!isSetTankCWaterDrainSurface){tankC.waterDrainSurface = (AreSame(tankC.waterDrainSurface, 0.2)) ? 0.2 : tankC.waterDrainSurface + 0.01; return;}
}

//function to subtract one from current set value if user press "-" button
void pressMinusBtn(Event& e ){
  Button& b = *e.button;
  //draw rect where value is on screen to aviod overlap
  M5.Lcd.fillRoundRect(100, 80, 120, 80,15, LIGHTGREY) ;
  if (!isSetMode) { mode -=1; if(mode % 4 == 0) mode = 3; return;}
  if (!isSetTankAHeight) {tankA.height = (tankA.height == 1) ? 1 : tankA.height - 1; return;}
  if (!isSetTankAWidth) {tankA.width = (tankA.width == 1) ? 1 : tankA.width - 1; return;}
  if (!isSetTankALength) {tankA.length = (tankA.length == 1) ? 1 : tankA.length - 1; return;}
  if (!isSetTankAWaterHeight){tankA.waterHeight = (AreSame(tankA.waterHeight, 0)) ? 0.0 : tankA.waterHeight - 0.1;
    tankA.waterHeight = (tankA.waterHeight < 0) ? 0 : tankA.waterHeight; return;}
  if (!isSetTankAWaterSupply){tankA.waterSupply = (AreSame(tankA.waterSupply, 0)) ? 0.00 : tankA.waterSupply - 0.01;
    tankA.waterSupply = (tankA.waterSupply < 0) ? 0 : tankA.waterSupply; return;};
  if (!isSetTankAWaterDrainSurface){tankA.waterDrainSurface = (AreSame(tankA.waterDrainSurface, 0)) ? 0.00 : tankA.waterDrainSurface - 0.01;
    tankA.waterDrainSurface = (tankA.waterDrainSurface < 0) ? 0.00 : tankA.waterDrainSurface; return;}
  if (!isSetTankBHeight) {tankB.height = (tankB.height == 1) ? 1 : tankB.height - 1; return;}
  if (!isSetTankBWidth) {tankB.width = (tankB.width == 1) ? 1 : tankB.width - 1; return;}
  if (!isSetTankBLength) {tankB.length = (tankB.length == 1) ? 1 : tankB.length - 1; return;}
  if (!isSetTankBWaterHeight){tankB.waterHeight = (AreSame(tankB.waterHeight, 0)) ? 0.0 : tankB.waterHeight - 0.1;
    tankB.waterHeight = (tankB.waterHeight < 0) ? 0 : tankB.waterHeight; return;}
  if (!isSetTankBWaterSupply){tankB.waterSupply = (AreSame(tankB.waterSupply, 0)) ? 0.00 : tankB.waterSupply - 0.01;
    tankB.waterSupply = (tankB.waterSupply < 0) ? 0 : tankB.waterSupply; return;};
  if (!isSetTankBWaterDrainSurface){tankB.waterDrainSurface = (AreSame(tankB.waterDrainSurface, 0)) ? 0.00 : tankB.waterDrainSurface - 0.01;
    tankB.waterDrainSurface = (tankB.waterDrainSurface < 0) ? 0.00 : tankB.waterDrainSurface; return;} 
  if (!isSetTankCHeight) {tankC.height = (tankC.height == 1) ? 1 : tankC.height - 1; return;}
  if (!isSetTankCWidth) {tankC.width = (tankC.width == 1) ? 1 : tankC.width - 1; return;}
  if (!isSetTankCLength) {tankC.length = (tankC.length == 1) ? 1 : tankC.length - 1; return;}
  if (!isSetTankCWaterHeight){tankC.waterHeight = (AreSame(tankC.waterHeight, 0)) ? 0.0 : tankC.waterHeight - 0.1;
    tankC.waterHeight = (tankC.waterHeight < 0) ? 0 : tankC.waterHeight; return;}
  if (!isSetTankCWaterSupply){tankC.waterSupply = (AreSame(tankC.waterSupply, 0)) ? 0.00 : tankC.waterSupply - 0.01;
    tankC.waterSupply = (tankC.waterSupply < 0) ? 0 : tankC.waterSupply; return;};
  if (!isSetTankCWaterDrainSurface){tankC.waterDrainSurface = (AreSame(tankC.waterDrainSurface, 0)) ? 0.00 : tankC.waterDrainSurface - 0.01;
    tankC.waterDrainSurface = (tankC.waterDrainSurface < 0) ? 0.00 : tankC.waterDrainSurface; return;}  
}

//function to change current set value when user press next button
void pressNextBtn(Event& e){
  Button& b = *e.button;
  M5.Lcd.clearDisplay();
  drawBtns = true;
  if(!isSetMode) {
    isSetMode = true;
    plusBtn.set(0,0,0,0);  
    minusBtn.set(0,0,0,0);
    nextBtn.set(0,0,0,0);
    plusBtn.setLabel("");
    minusBtn.setLabel("");
    nextBtn.setLabel("");   
    if(mode != 3) {
      isReset = false;
      tankA.waterSupplyValve1In = tankA.waterSupply;
      tankB.waterSupplyValve1In = tankB.waterSupply;
      tankA.waterHeight = 0;
      tankB.waterHeight = 0;
      tankC.waterHeight = 0;
      tankC.valveOut = 0;
      cascadeType = 2;
      startTaskTime = millis();
    }
    return;
  }   
  if (!isSetTankAHeight) {isSetTankAHeight = true; currentSet = "szerokosc"; return;}
  if (!isSetTankAWidth) {isSetTankAWidth = true; currentSet = "dlugosc"; return;}
  if (!isSetTankALength){isSetTankALength = true; tankA.waterQuantity = tankA.waterHeight * tankA.length * tankA.width; currentSet = "poziom wody"; return;}
  if (!isSetTankAWaterHeight){
    isSetTankAWaterHeight = true;
    currentSet = "dopływ wody";
    if(tankA.waterHeight >= 0.9 * tankA.height)
      tankA.alarm = true;
     return;}
  if (!isSetTankAWaterSupply){isSetTankAWaterSupply = true; tankA.previousWaterSupply = tankA.waterSupply; tankA.waterSupplyValve1In = tankA.waterSupply;  currentSet = "powierzchnia odpływu wody"; return;}
  if (!isSetTankAWaterDrainSurface){isSetTankAWaterDrainSurface = true; tankA.previousWaterDrainSurface = tankA.waterDrainSurface; currentSet = "wysokosc"; currentSetTank="zbiornik B"; return;}
  if (!isSetTankBHeight) {isSetTankBHeight = true; currentSet = "szerokosc"; return;}
  if (!isSetTankBWidth) {isSetTankBWidth = true; currentSet = "dlugosc"; return;}
  if (!isSetTankBLength){isSetTankBLength = true; currentSet = "poziom wody"; return;}
  if (!isSetTankBWaterHeight){
    isSetTankBWaterHeight = true;
    tankB.waterQuantity = tankB.waterHeight * tankB.length * tankB.width;
    currentSet = "dopływ wody";
    if (cascadeType == 1) {isSetTankBWaterSupply = true; currentSet = "powierzchnia odpływu wody";}
    return;}
  if (!isSetTankBWaterSupply){isSetTankBWaterSupply = true; tankB.previousWaterSupply = tankB.waterSupply; tankB.waterSupplyValve1In = tankB.waterSupply; currentSet = "powierzchnia odpływu wody"; return;}
  if (!isSetTankBWaterDrainSurface){isSetTankBWaterDrainSurface = true; tankB.previousWaterDrainSurface = tankB.waterDrainSurface; currentSet = "wysokosc"; currentSetTank="zbiornik C"; return;}
  if (!isSetTankCHeight) {isSetTankCHeight = true; currentSet = "szerokosc"; return;}
  if (!isSetTankCWidth) {isSetTankCWidth = true; currentSet = "dlugosc"; return;}
  if (!isSetTankCLength){isSetTankCLength = true; tankC.waterQuantity = tankC.waterHeight * tankB.length * tankC.width; currentSet = "poziom wody"; return;}
  if (!isSetTankCWaterHeight){isSetTankCWaterHeight = true; isSetTankCWaterSupply = true; currentSet = "powierzchnia odplywu wody"; return;}
  if (!isSetTankCWaterDrainSurface){
    tankC.previousWaterDrainSurface = tankC.waterDrainSurface;
    isSetCascadeType = false;
    isSetTankAHeight = false;
    isSetTankAWidth = false;
    isSetTankALength = false;
    isSetTankAWaterHeight = false;
    isSetTankAWaterSupply = false;
    isSetTankAWaterDrainSurface = false;
    isSetTankBHeight = false;
    isSetTankBWidth = false;
    isSetTankBLength = false;
    isSetTankBWaterHeight = false;
    isSetTankBWaterSupply = false;
    isSetTankBWaterDrainSurface = false;
    isSetTankCHeight = false;
    isSetTankCWidth = false;
    isSetTankCLength = false;
    isSetTankCWaterHeight = false;
    isSetTankCWaterSupply = false;
    isSetTankCWaterDrainSurface = false;
    isSetMode = false;
    isReset = false;
    currentSet = "Wybierz tryb pracy";
    currentSetTank="zbiornik A";
    plusBtn.set(0,0,0,0);  
    minusBtn.set(0,0,0,0);
    nextBtn.set(0,0,0,0);
    plusBtn.setLabel("");
    minusBtn.setLabel("");
    nextBtn.setLabel("");
    M5.lcd.setCursor(0,20);
    M5.Lcd.clearDisplay(); 
    drawBtns = true;
    startTaskTime = millis(); 
  }
}

//function that sets all values to defoule when user press reset button
void pressResetBtn(Event& e){
  Button& b = *e.button;  
  drawBtns = true;
  isSetMode = false;
  isReset = true;  
  upBtn.set(0, 0, 0, 0);
  downBtn.set(0,0, 0, 0);
  resetBtn.set(0,0,0,0);
  detailsBtn.set(0, 0, 0, 0); 
  plus2Btn.set(0, 0, 0,0);
  minus2Btn.set(0, 0, 0,0);   
  upBtn.setLabel("");
  downBtn.setLabel("");
  resetBtn.setLabel("");
  currentSetTank="zbiornik A";
  currentProperty = 1;
  cascadeType = -1;
  page = 1; 
  drawTank = true;
  tankA = {.height = 1.0, .width = 1.0, .length = 1.0, .waterHeight= 0.5, .waterSupply = 0.03, .waterDrainSurface = 0.01, .alarm = false, .state = 1, .valve1In = 1, .valve2In = 0, .valveOut = 1, .heightLow = 0, .heightMiddle =0, .heightHigh = 0};
  tankB = {.height = 1.0, .width = 1.0, .length = 1.0, .waterHeight= 0.5, .waterSupply = 0.03, .waterDrainSurface = 0.01, .alarm = false, .state = 1, .valve1In = 1, .valve2In = 0, .valveOut = 1, .heightLow = 0, .heightMiddle =0, .heightHigh = 0};
  tankC = {.height = 1.0, .width = 1.0, .length = 1.0, .waterHeight= 0.5, .waterSupply = 0.03, .waterDrainSurface = 0.01, .alarm = false, .state = 1, .valve1In = 1, .valve2In = 1, .valveOut = 1, .heightLow = 0, .heightMiddle =0, .heightHigh = 0}; 
  M5.Lcd.clearDisplay();  
}

//function changes current display property
void pressUpBtn(Event& e){
  drawBtns = true;
  Button& b = *e.button;
  //draw rect where value is on screen to aviod overlap
  if(page == 1) M5.Lcd.fillRoundRect(200,100, 120, 90,15, LIGHTGREY) ; 
  if(page == 2) M5.Lcd.fillRoundRect(50, 80, 220, 80,15, LIGHTGREY) ;
  if(page == 2) M5.Lcd.fillRect(0, 35, 320, 40, BLACK) ;
  currentProperty = (currentProperty + 1);
  if(currentProperty == 9) currentProperty = 1;   
}

//function changes current display property
void pressDownBtn(Event& e){
  drawBtns = true;
  Button& b = *e.button;
  //draw rect where value is on screen to aviod overlap
  if(page == 1) M5.Lcd.fillRoundRect(200,100, 120, 90,15, LIGHTGREY) ; 
  if(page == 2) M5.Lcd.fillRoundRect(50, 80, 220, 80,15, LIGHTGREY) ;
  if(page == 2) M5.Lcd.fillRect(0, 35, 320, 40, BLACK) ; 
  currentProperty = (currentProperty - 1);
  if(currentProperty == 0) currentProperty = 8;   
}

//function that changes move user to deteil screen when they press greay area with property 
void pressDetailsBtn(Event& e){
  Button& b = *e.button;
  M5.Lcd.clear(); 
  page =2;
  drawBtns = true; 
  drawTank = true;
}

//funtion to go back to main screen after user click back button
void pressGoBackBtn(Event& e){
  Button& b = *e.button;
  goBackBtn.set(0, 0, 0, 0);
  goBackBtn.setLabel("");
  M5.Lcd.clear();
  page = 1;
  drawBtns = true; 
  drawTank = true;   
}

//function to subtract one from current display value if user press "-" button. it is for when tank cascade is aready set so user can only change water supply of drain surface
void pressMinus2Btn(Event& e ){
  //draw rect where value is on screen to aviod overlap
  M5.Lcd.fillRoundRect(50, 80, 220, 80,15, LIGHTGREY) ;
  if(currentProperty == 5){
    if(currentSetTank == "zbiornik A"){
      tankA.waterSupply = (AreSame(tankA.waterSupply, 0)) ? 0.00 : tankA.waterSupply - 0.01;
      tankA.waterSupply = (tankA.waterSupply < 0) ? 0 : tankA.waterSupply;   
    }  
    if(currentSetTank == "zbiornik B" && cascadeType == 2){
      tankB.waterSupply = (AreSame(tankB.waterSupply, 0)) ? 0.00 : tankB.waterSupply - 0.01; 
      tankB.waterSupply = (tankB.waterSupply < 0) ? 0 : tankB.waterSupply;
    }  
  }
  if(currentProperty == 6){
    if(currentSetTank == "zbiornik A"){
      tankA.waterDrainSurface = (AreSame(tankA.waterDrainSurface, 0)) ? 0.00 : tankA.waterDrainSurface - 0.01;
      tankA.waterDrainSurface = (tankA.waterDrainSurface < 0) ? 0.00 : tankA.waterDrainSurface;            
    } 
    if(currentSetTank == "zbiornik B"){
      tankB.waterDrainSurface = (AreSame(tankB.waterDrainSurface, 0)) ? 0.00 : tankB.waterDrainSurface - 0.01;
      tankB.waterDrainSurface = (tankB.waterDrainSurface < 0) ? 0.00 : tankB.waterDrainSurface;      
    } 
    if(currentSetTank == "zbiornik C"){
      tankC.waterDrainSurface = (AreSame(tankC.waterDrainSurface, 0)) ? 0.00 : tankC.waterDrainSurface - 0.01; 
      tankC.waterDrainSurface = (tankC.waterDrainSurface < 0) ? 0.00 : tankC.waterDrainSurface;        
    } 
  }
}  

//function to add one to current display value if user press "+" button. it is for when tank cascade is aready set so user can only change water supply of drain surface
void pressPlus2Btn(Event& e ){
  //draw rect where value is on screen to aviod overlap
  M5.Lcd.fillRoundRect(50, 80, 220, 80,15, LIGHTGREY) ;
  if(currentProperty == 5){
    if(currentSetTank == "zbiornik A") {tankA.waterSupply = (AreSame(tankA.waterSupply, 0.1)) ? 0.10 : tankA.waterSupply + 0.01; tankA.waterSupplyValve1In = tankA.waterSupply;} 
    if(currentSetTank == "zbiornik B"  && cascadeType == 2) {tankB.waterSupply = (AreSame(tankB.waterSupply, 0.1)) ? 0.10 : tankB.waterSupply + 0.01; tankB.waterSupplyValve1In = tankB.waterSupply;}   
  }
  if(currentProperty == 6){
    if(currentSetTank == "zbiornik A") tankA.waterDrainSurface = (AreSame(tankA.waterDrainSurface, 0.2)) ? 0.2 : tankA.waterDrainSurface + 0.01; 
    if(currentSetTank == "zbiornik B") tankB.waterDrainSurface = (AreSame(tankB.waterDrainSurface, 0.2)) ? 0.2 : tankB.waterDrainSurface + 0.01;
    if(currentSetTank == "zbiornik C") tankC.waterDrainSurface = (AreSame(tankC.waterDrainSurface, 0.2)) ? 0.2 : tankC.waterDrainSurface + 0.01;
  }
}

//below functions are for drawing buttons and displaying values on screen 
void drawSetCascadeBtns(){
  M5.Lcd.setCursor(20,20);
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("Wybierz rodzaj kaskady zbiornikow:");
  cascadeTypeBtn1.set(0, 25, 320, 105);
  cascadeTypeBtn2.set(0, 105 +25, 320, 105); 
  cascadeTypeBtn1.setLabel("A->B->C"); 
  cascadeTypeBtn2.setLabel("A->C B->C"); 
  cascadeTypeBtn1.draw();
  cascadeTypeBtn2.draw();
  drawBtns = false;  
}

void drawOtherSetBtns(){
  M5.Lcd.setCursor(90,20);
  M5.Lcd.setTextSize(1);
  if(isSetMode) { M5.Lcd.printf("%s \n",currentSetTank.c_str()); M5.Lcd.setCursor(90,40);}
  if(!isSetMode) {M5.Lcd.setTextSize(2); M5.Lcd.setCursor(0,30);}
  M5.Lcd.printf("%s:",currentSet.c_str());
  plusBtn.set(240, 80, 80, 80);
  minusBtn.set(0, 80, 80, 80);
  nextBtn.set(80, 180, 160, 60);
  minusBtn.setLabel("-");
  plusBtn.setLabel("+");
  if(!isSetMode) { plusBtn.setLabel(">"); minusBtn.setLabel("<");}  
  nextBtn.setLabel("zatwierdz");     
  plusBtn.setTextSize(3);  
  minusBtn.setTextSize(3);        
  plusBtn.draw();
  minusBtn.draw();
  nextBtn.draw();
  M5.Lcd.fillRoundRect(100, 80, 120, 80,15, LIGHTGREY) ;
  drawBtns = false;  
}

void drawBtnsFun(){
  drawBtns = false;
  if(page == 1){
    detailsBtn.set(200, 100, 120, 90);
    upBtn.set(210, 40, 100, 40);
    downBtn.set(210,200, 100, 40);
    resetBtn.set(210,0,100,40); 
    upBtn.setTextSize(1);
    downBtn.setTextSize(1); 
    upBtn.setLabel("Nastepna");
    downBtn.setLabel("Poprzednia");
    resetBtn.setLabel("RESET");
    upBtn.draw();
    downBtn.draw();
    resetBtn.draw();
    detailsBtn.draw();    
  }
  if(page == 2){        
    detailsBtn.set(0, 0, 0, 0); 
    resetBtn.set(0, 0, 0, 0);
    downBtn.set(0, 80, 40,40);
    downBtn.setLabel("<");
    upBtn.setTextSize(2);
    downBtn.setTextSize(2);      
    upBtn.setLabel(">");
    upBtn.set(280, 80, 40,40);
    goBackBtn.set(80, 180, 160, 60);
    plus2Btn.set(0, 0, 0, 0);
    minus2Btn.set(0, 0, 0, 0);
    goBackBtn.setLabel("powrot");
    goBackBtn.draw();  
    upBtn.draw();
    downBtn.draw();
    //draw button only if current tank dont have alarm and current property is drain surface of water supply. If current property is water supply
    //show button only if current tank is A or current tank is B and cascade type is 2.   
    if(((tankA.alarm == false && currentSetTank == "zbiornik A") ||
      (tankB.alarm == false && currentSetTank == "zbiornik B") || 
      (tankC.alarm == false && currentSetTank == "zbiornik C")) &&
      (currentProperty == 5 && (currentSetTank == "zbiornik A" || (currentSetTank == "zbiornik B" && cascadeType == 2) )
      || currentProperty == 6)){            
      plus2Btn.set(0, 120, 40, 40);
      minus2Btn.set(280, 120, 40, 40);
      plus2Btn.setTextSize(2);  
      minus2Btn.setTextSize(2);
      plus2Btn.draw();
      minus2Btn.draw();
    }else{
      plus2Btn.set(0, 0, 0, 0);
      minus2Btn.set(0, 0, 0, 0);
      //draw rects where buttons are on screen hide them
      M5.Lcd.fillRect(0, 120, 40, 40, BLACK);
      M5.Lcd.fillRect(280, 120, 40, 40, BLACK);
    }
  }
}

void drawRectFun(){
  //draw rect where value is on screen to aviod overlap
  if (page == 1) M5.Lcd.fillRoundRect(200, 100, 120, 90,15, LIGHTGREY) ;
  if (page == 2) M5.Lcd.fillRoundRect(50, 80, 220, 80,15, LIGHTGREY) ; 
  drawRect = false;    
}

void displaySettingValue(float value, String unit){
  M5.Lcd.setCursor(90,60);  
  M5.Lcd.setTextSize(1);     
  M5.Lcd.printf("jednostka: %s", unit.c_str());
  M5.Lcd.setTextSize(3);        
  M5.Lcd.setCursor(110,140);       
  M5.Lcd.printf("%.0f", value);
}

void displaySettingMode(int value){
  M5.Lcd.setTextSize(3);        
  M5.Lcd.setCursor(110,140);      
  M5.Lcd.printf("%d", value);  
}

void displaySettingValueWithMorePrecision(float value, String unit){
  M5.Lcd.setCursor(90,60);  
  M5.Lcd.setTextSize(1);     
  M5.Lcd.printf("jednostka: %s", unit.c_str());
  M5.Lcd.setTextSize(2);        
  M5.Lcd.setCursor(110,140);      
  M5.Lcd.printf("%.2f", value);
}

void displayCurrentProperty(){
  if(page == 1) M5.Lcd.setCursor(210, 120);
  if(page == 2) M5.Lcd.setCursor(0, 60);
  if(page == 1) M5.Lcd.setTextSize(1); 
  if(page == 2) M5.Lcd.setTextSize(2);
  
  M5.Lcd.printf(propertyNames[currentProperty - 1]);   
  if(page == 2 && currentProperty >= 1 && currentProperty <= 4) M5.Lcd.printf("(m)");
  if(page == 2 && (currentProperty == 5 || currentProperty == 8)) M5.Lcd.printf("(m^3/s)"); 
  if(page == 2 && currentProperty == 6) M5.Lcd.printf("(m^2)");
  if(page == 2 && currentProperty == 7) M5.Lcd.printf("(m^3)");  
  if(page == 1) M5.Lcd.setCursor(205, 160);
  if(page == 2) M5.Lcd.setCursor(80, 140);
  M5.Lcd.setTextSize(2); 
  if(page == 2) M5.Lcd.setTextSize(3);
  if(currentProperty == 1){
    if(currentSetTank =="zbiornik A") M5.Lcd.printf("%.0f",tankA.height);
    if(currentSetTank =="zbiornik B") M5.Lcd.printf("%.0f",tankB.height);
    if(currentSetTank =="zbiornik C") M5.Lcd.printf("%.0f",tankC.height); 
    if(page == 1) M5.Lcd.printf(" m");      
  }; 
  if(currentProperty == 2){    
    if(currentSetTank =="zbiornik A") M5.Lcd.printf("%.0f",tankA.width);
    if(currentSetTank =="zbiornik B") M5.Lcd.printf("%.0f",tankB.width);
    if(currentSetTank =="zbiornik C") M5.Lcd.printf("%.0f",tankC.width);     
    if(page == 1) M5.Lcd.printf(" m"); 
  } 
  if(currentProperty == 3){    
    if(currentSetTank =="zbiornik A") M5.Lcd.printf("%.0f",tankA.length);
    if(currentSetTank =="zbiornik B") M5.Lcd.printf("%.0f",tankB.length);
    if(currentSetTank =="zbiornik C") M5.Lcd.printf("%.0f",tankC.length);
    if(page == 1) M5.Lcd.printf(" m");      
  }
  if(currentProperty == 4){
    if(currentSetTank =="zbiornik A") M5.Lcd.printf("%.2f",tankA.waterHeight);
    if(currentSetTank =="zbiornik B") M5.Lcd.printf("%.2f",tankB.waterHeight);
    if(currentSetTank =="zbiornik C") M5.Lcd.printf("%.2f",tankC.waterHeight); 
    if(page == 1) M5.Lcd.printf(" m");     
  }
  if(currentProperty == 5){    
    if(currentSetTank =="zbiornik A") M5.Lcd.printf("%.3f",tankA.waterSupply);
    if(currentSetTank =="zbiornik B") M5.Lcd.printf("%.3f",tankB.waterSupply); 
    if(currentSetTank =="zbiornik C") M5.Lcd.printf("%.3f",tankC.waterSupply);      
  }
  if(currentProperty == 6){
    if(currentSetTank =="zbiornik A") M5.Lcd.printf("%.3f",tankA.waterDrainSurface);
    if(currentSetTank =="zbiornik B") M5.Lcd.printf("%.3f",tankB.waterDrainSurface); 
    if(currentSetTank =="zbiornik C") M5.Lcd.printf("%.3f",tankC.waterDrainSurface);      
  } 
  if(currentProperty == 7){
    if(currentSetTank =="zbiornik A") M5.Lcd.printf("%.2f",tankA.waterQuantity);
    if(currentSetTank =="zbiornik B") M5.Lcd.printf("%.2f",tankB.waterQuantity);
    if(currentSetTank =="zbiornik C") M5.Lcd.printf("%.2f",tankC.waterQuantity);      
  }
  if(currentProperty == 8){ 
    if(currentSetTank =="zbiornik A") M5.Lcd.printf("%.3f",tankA.waterDrain);
    if(currentSetTank =="zbiornik B") M5.Lcd.printf("%.3f",tankB.waterDrain);
    if(currentSetTank =="zbiornik C") M5.Lcd.printf("%.3f",tankC.waterDrain);      
  }  
}

void displayTank(){
    if(!isReset && drawTank){
      (cascadeType ==2 && currentSetTank == "zbiornik C") ? M5.Lcd.drawJpg(IMGB,IMGBSize,0,40,200,200) : M5.Lcd.drawJpg(IMG,IMGSize,0,40,200,200);    
      drawTank = false;      
    } 
}

void displayWaterLevel(){
  if(millis() - displayWaterLevelTime >= 500 || M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()){
    displayWaterLevelTime = millis();    
    M5.Lcd.fillRect(33, 109, 156, 64, WHITE);
    if (currentSetTank =="zbiornik A") M5.Lcd.fillRect(33, 133 + 40 - floor(63 *(tankA.waterHeight/tankA.height)), 156, floor(63 *(tankA.waterHeight/tankA.height)), BLUE);
    if (currentSetTank =="zbiornik B") M5.Lcd.fillRect(33, 133 + 40 - floor(63 *(tankB.waterHeight/tankB.height)), 156, floor(63 *(tankB.waterHeight/tankB.height)), BLUE);
    if (currentSetTank =="zbiornik C") M5.Lcd.fillRect(33, 133 + 40 - floor(63 *(tankC.waterHeight/tankC.height)), 156, floor(63 *(tankC.waterHeight/tankC.height)), BLUE);  
  }
}

void displayAlarm(){
  alarmCount = (alarmCount + 1) % 500;
    if(alarmCount % 500 < 250){   
    if(currentSetTank == "zbiornik A" && tankA.alarm){ M5.Lcd.fillRect(20, 180, 25, 25, YELLOW); return;}
    if(currentSetTank == "zbiornik B" && tankB.alarm){ M5.Lcd.fillRect(20, 180, 25, 25, YELLOW); return;}
    if(currentSetTank == "zbiornik C" && tankC.alarm){ M5.Lcd.fillRect(20, 180, 25, 25, YELLOW); return;}
  }else{
    M5.Lcd.fillRect(20, 180, 25, 25, WHITE);
     return;    
  }
  M5.Lcd.fillRect(20, 180, 25, 25, WHITE);  
}

void displayDiodeSenors(){
  if(currentSetTank != "zbiornik C") M5.Lcd.fillCircle(150, 90, 7, WHITE);
  if(currentSetTank == "zbiornik A"){
    if(tankA.valve1In) M5.Lcd.fillCircle(50, 90, 7, GREEN);
    else M5.Lcd.fillCircle(50, 90, 7, RED);   
    if(tankA.valveOut) M5.Lcd.fillCircle(150, 220, 7, GREEN);
    else M5.Lcd.fillCircle(150, 220, 7, RED);
  }
  if(currentSetTank == "zbiornik B"){
    if(tankB.valve1In) M5.Lcd.fillCircle(50, 90, 7, GREEN);
    else M5.Lcd.fillCircle(50, 90, 7, RED);
    if(tankB.valveOut) M5.Lcd.fillCircle(150, 220, 7, GREEN);
    else M5.Lcd.fillCircle(150, 220, 7, RED);    
  }
  if(currentSetTank == "zbiornik C"){
    if(tankC.valve1In) M5.Lcd.fillCircle(50, 90, 7, GREEN);
    else M5.Lcd.fillCircle(50, 90, 7, RED);
    if(tankC.valve2In) M5.Lcd.fillCircle(150, 90, 7, GREEN);
    else M5.Lcd.fillCircle(150, 90, 7, RED);       
    if(tankC.valveOut) M5.Lcd.fillCircle(150, 220, 7, GREEN);
    else M5.Lcd.fillCircle(150, 220, 7, RED);
  }
}

//task to calculate water flow in tanks  
void taskCalculateWaterFlow(void* pvParameters){
  while(1){
    if(millis() - calculateWaterFlowTime >= 100){
      calculateWaterFlowTime = millis();
      if(!isReset){
        tankWaterFlow(&tankA);
        //Serial.printf("%d,%f,%d \n", millis() - startTaskTime, tankA.waterHeight, tankA.state); //for tests
        tankAlarm(&tankA, 0.9, 0.7);
        if(cascadeType == 1){
           tankB.waterSupplyValve1In = tankA.waterDrain;                     
        }        
        tankWaterFlow(&tankB);
        //Serial.printf("%d,%f,%d \n", millis() - startTaskTime, tankB.waterHeight, tankB.state); //for tests
        tankAlarm(&tankB, 0.9, 0.7);
        if(cascadeType == 1){
           tankC.waterSupplyValve1In = tankB.waterDrain;
           tankC.waterSupply = tankC.waterSupplyValve1In;                     
        }
        if(cascadeType == 2){
           tankC.waterSupplyValve2In = tankB.waterDrain;  
           tankC.waterSupplyValve1In = tankA.waterDrain;
           tankC.waterSupply = tankC.waterSupplyValve1In + tankC.waterSupplyValve2In;                     
        }
        tankWaterFlow(&tankC); 
        //Serial.printf("%d,%f,%d \n", millis() - startTaskTime, tankC.waterHeight, tankC.state); //for tests
        tankAlarm(&tankC, 0.9, 0.7);
      }                 
    }
    if(millis() - calculateWaterFlowTime2 >= 300 && !isReset){
      drawRect = true;
      calculateWaterFlowTime2 = millis();  
    }    
  }
}

//functoin to calculate water level in each cycle in tank which is passed as agrument
void tankWaterFlow(tank *currentTank){
  (*currentTank).waterDrain = (*currentTank).valveOut * (*currentTank).waterDrainSurface * sqrt( 2 * G * (*currentTank).waterHeight) * 0.1;
  if((*currentTank).waterDrain > ((*currentTank).waterQuantity)){
    (*currentTank).waterDrain = ((*currentTank).waterQuantity);
    (*currentTank).waterQuantity = 0;
  }else
    (*currentTank).waterQuantity = (*currentTank).waterHeight * (*currentTank).width * (*currentTank).length -
    (*currentTank).waterDrain + (*currentTank).waterSupplyValve1In * 0.1 * (*currentTank).valve1In +
    (*currentTank).waterSupplyValve2In * 0.1 * (*currentTank).valve2In;   
  (*currentTank).waterHeight = (*currentTank).waterQuantity/ ((*currentTank).width * (*currentTank).length); 
  (*currentTank).waterDrain = (*currentTank).waterDrain * 10;
  if ((*currentTank).waterHeight > 0.3 * (*currentTank).height) (*currentTank).heightLow = 1;
  else (*currentTank).heightLow = 0;
  if ((*currentTank).waterHeight > 0.5 * (*currentTank).height) (*currentTank).heightMiddle = 1;
  else (*currentTank).heightMiddle = 0;
  if ((*currentTank).waterHeight > 0.7 * (*currentTank).height) (*currentTank).heightHigh = 1;
  else (*currentTank).heightHigh = 0;       
}

//function to check if water level is too high in tank. When it is drain surface is changed that water drain is greater than water supply 
void tankAlarm(tank *currentTank, double alarmOn, double alarmOff ){
  if(!(*currentTank).alarm && (*currentTank).waterHeight >= alarmOn * (*currentTank).height){
    (*currentTank).alarm = true;
    (*currentTank).previousWaterDrainSurface = (*currentTank).waterDrainSurface;
    (*currentTank).waterDrainSurface = (*currentTank).waterSupply / 3; 
    drawBtns = true;
  }                
  if((*currentTank).alarm && (*currentTank).waterHeight <= alarmOff * (*currentTank).height){
    (*currentTank).alarm = false;
    (*currentTank).waterDrainSurface = (*currentTank).previousWaterDrainSurface;
    drawBtns = true;
  } 
  if((*currentTank).alarm)  (*currentTank).waterDrainSurface = (*currentTank).waterSupply / 3;   
}

//task to execute preset task. when i try to run it useing xTaskCreatePinnedToCore() like taskCalculateWaterFlow device crashed.  
void carryOnPresetTask(){
    if(!isReset && millis() - CarryOnPresetTaskTime >= 100){
      CarryOnPresetTaskTime = millis();
      if (mode == 1) switchCaseForMode1();
      if (mode == 2) switchCaseForMode2();
    }   
}

//carries out task from attached file "wyklad_USCtankC.valve1In_.pdf";
void switchCaseForMode1(){
  switch(tankA.state) {
    case 1: 
      tankA.valve1In = 1; 
      if(tankA.heightLow){tankA.timeA = 30; tankA.state = 2;}
      break;
    case 2: 
      tankA.valve1In = 1; 
      if(tankA.heightHigh){tankA.timeA = 50; tankA.state = 4;}
      else if(!tankA.timeA){tankA.timeA = 20; tankA.state = 3;}
      else if(!tankA.heightLow) tankA.state = 1;
      break;
    case 3: 
      tankA.valve1In = 0; 
      if(!tankA.timeA){tankA.timeA = 30; tankA.state = 2;}
      else if(!tankA.heightLow) tankA.state = 1;
      break;
    case 4: 
      tankA.valve1In = 0; 
      if(!tankA.timeA && tankA.heightLow) tankA.state = 5;
      else if(!tankA.timeA && !tankA.heightLow) tankA.state = 1;
      break;
    case 5: 
      tankA.valve1In = 0; 
      if(!tankA.heightLow) tankA.state = 1;
      break;
  }
  if (tankA.timeA) --tankA.timeA;
  switch(tankB.state) {
    case 1: 
      tankB.valve1In = 1; 
      if(tankB.heightLow){tankB.timeA = 30; tankB.timeB = 80; tankB.state = 2;}
      break;
    case 2: 
      tankB.valve1In = 1; 
      if(!tankB.timeB || tankB.heightHigh) tankB.state = 4;
      else if(!tankB.timeA){tankB.timeA=20; tankB.state=3;}
      else if(!tankB.heightLow) tankB.state = 1;
      break;
    case 3: 
      tankB.valve1In = 0; 
      if(!tankB.timeA){tankB.timeA = 30; tankB.state = 2;}
      else if(!tankB.timeB) tankB.state = 4;
      else if(!tankB.heightLow) tankB.state = 1;
      break;
    case 4: 
      tankB.valve1In=0;
      if(!tankB.heightLow) tankB.state=1;
      break;
  } 
  if (tankB.timeA) --tankB.timeA;
  if (tankB.timeB) --tankB.timeB;
  switch(tankC.state) {
    case 1:
      tankC.valve1In = 1;
      tankC.valve2In = 1; 
      tankC.valveOut = 0;
      if(tankC.heightLow) {tankC.timeA = 30; tankC.state = 2;}
      break;
    case 2: 
      tankC.valve1In = 1; 
      tankC.valve2In = 0; 
      tankC.valveOut = 0;
      if(tankC.heightHigh) {tankC.timeA = 50; tankC.state = 4;}
      else if(!tankC.timeA) {tankC.timeA = 30; tankC.state = 3;}
      break;
    case 3:
      tankC.valve1In = 0; 
      tankC.valve2In = 1; 
      tankC.valveOut = 0;
      if(tankC.heightHigh) {tankC.timeA = 50; tankC.state = 4;}
      else if(!tankC.timeA) {tankC.timeA = 20; tankC.state = 2;}
      break;
    case 4: 
      tankC.valve1In = 0;
      tankC.valve2In = 0; 
      tankC.valveOut = 0;
      if(!tankC.timeA) {tankC.timeA = 50; tankC.state = 5;}
      break;
    case 5:
      tankC.valve1In = 0; 
      tankC.valve2In = 0; 
      tankC.valveOut = 1;
      if(!tankC.timeA && tankC.heightLow) tankC.state = 6;
      else if(!tankC.timeA && !tankC.heightLow) tankC.state = 1;
      break;
    case 6:
      if(!tankC.heightLow) tankC.state = 1; 
  }
  if (tankC.timeA) --tankC.timeA;
  tankA.valveOut = tankC.valve1In;
  tankB.valveOut = tankC.valve2In;
}

//carries out task from attached file "Tryb pracy 2.docx"
void switchCaseForMode2(){
  switch(tankA.state) {
    case 1:
      tankA.valve1In = 1;
      if(tankA.heightLow){tankA.state = 2; tankA.timeA = 30;}
      break;
    case 2:
      tankA.valve1In = 1;
      if(!tankA.heightLow) tankA.state = 1;
      if(tankA.heightMiddle){tankA.timeA = 50; tankA.state = 4;}
      if(!tankA.timeA){tankA.state = 3; tankA.timeA = 30;}
      break;
    case 3:
      tankA.valve1In = 0;
      if(!tankA.heightLow) tankA.state = 1;
      if(!tankA.timeA) {tankA.timeA = 30; tankA.state = 2;}
      break;
    case 4:
      tankA.valve1In = 1;
      if(!tankA.heightLow) tankA.state = 1;
      if(!tankA.timeA || tankA.heightHigh){tankA.state = 5; tankA.timeA = 50;}
      break;
    case 5:
      tankA.valve1In = 0;
      if(!tankA.heightLow && !tankA.timeA) tankA.state = 1;
      if(!tankA.timeA) tankA.state = 6;
      break;
    case 6:
      tankA.valve1In = 0;
      if(!tankA.heightLow) tankA.state = 1;
  }
  if (tankA.timeA) --tankA.timeA;
  switch(tankB.state){
    case 1:
      tankB.valve1In = 1;
      if(tankB.heightLow){tankB.timeA = 30; tankB.state = 2;}
      break;
    case 2:
      tankB.valve1In = 1;
      if(!tankB.heightLow) tankB.state = 1;
      if(tankB.heightHigh){tankB.state = 4; tankB.timeA = 50;}
      if(!tankB.timeA){tankB.state = 3; tankB.timeA = 30;}
      break;
    case 3:
      tankB.valve1In = 0;
      if(!tankB.heightLow) tankB.state = 1;
      if(!tankB.timeA){ tankB.timeA = 30; tankB.state = 2;}
      break;
    case 4:
      tankB.valve1In = 0;
      if(!tankB.timeA && tankB.heightLow) tankB.state = 1;
      if(!tankB.timeA) tankB.state = 5;
      break;
    case 5:
      tankB.valve1In = 0;
      if(!tankB.heightLow) tankB.state = 1;
      break;       
  }
  if (tankB.timeA) --tankB.timeA;
  switch(tankC.state) {
    case 1:
      tankC.valve1In = 1;
      tankC.valve2In = 1;
      tankC.valveOut = 0;
      if(tankC.heightLow){tankC.timeA = 40; tankC.state = 2;}
      break;
    case 2:
      tankC.valve1In = 1;
      tankC.valve2In = 0;
      tankC.valveOut = 0;
      if(tankC.heightMiddle) {tankC.timeA = 30; tankC.state = 4;}
      if(!tankC.timeA) {tankC.timeA = 20; tankC.state = 3;}    
      break;
    case 3:
      tankC.valve1In = 0;
      tankC.valve2In = 1;
      tankC.valveOut = 0;
      if(tankC.heightMiddle) {tankC.timeA = 30; tankC.state = 4;}
      if(!tankC.timeA) {tankC.timeA = 40; tankC.state = 2;}
      break;
    case 4:
      tankC.valve1In = 0;
      tankC.valve2In = 1;
      tankC.valveOut = 0;
      if(tankC.heightHigh){tankC.state = 6; tankC.timeA = 40;}
      if(!tankC.timeA){tankC.timeA = 30; tankC.state = 5;}
      break;
    case 5:
      tankC.valve1In = 0;
      tankC.valve2In = 0;
      tankC.valveOut = 0; 
      if(!tankC.timeA){tankC.timeA = 30; tankC.state = 4;}
      break;
    case 6:
      tankC.valve1In = 0;
      tankC.valve2In = 0;
      tankC.valveOut = 0;
      if(!tankC.timeA){tankC.timeA = 30; tankC.state = 7;}
      break;
    case 7:
      tankC.valve1In = 0;
      tankC.valve2In = 0;
      tankC.valveOut = 1;
      if(!tankC.heightLow) tankC.state = 1;
      if(!tankC.timeA){tankC.timeA = 30; tankC.state = 8;} 
      break;
    case 8:
      tankC.valve1In = 0;
      tankC.valve2In = 0;
      tankC.valveOut = 0;
      if(!tankC.timeA){tankC.timeA = 30; tankC.state = 7;}
      break;       
  }
  if (tankC.timeA) --tankC.timeA;
  tankA.valveOut = tankC.valve1In;
  tankB.valveOut = tankC.valve2In;
}

void setup() {
  //add event handlers for buttons  
  cascadeTypeBtn1.addHandler(setCascadeType, E_RELEASE); 
  cascadeTypeBtn2.addHandler(setCascadeType, E_RELEASE);
  plusBtn.addHandler(pressPlusBtn, E_TOUCH );
  minusBtn.addHandler(pressMinusBtn, E_TOUCH );
  plus2Btn.addHandler(pressPlus2Btn, E_TOUCH );
  minus2Btn.addHandler(pressMinus2Btn, E_TOUCH );
  nextBtn.addHandler(pressNextBtn, E_TOUCH); 
  resetBtn.addHandler(pressResetBtn, E_TOUCH); 
  upBtn.addHandler(pressUpBtn, E_TOUCH);
  downBtn.addHandler(pressDownBtn, E_TOUCH);  
  detailsBtn.addHandler(pressDetailsBtn, E_TOUCH);
  goBackBtn.addHandler(pressGoBackBtn, E_TOUCH);  
  M5.begin(); 
  xTaskCreatePinnedToCore(
      taskCalculateWaterFlow,  //Function to implement the task.
      "taskCalculateWaterFlow",
      16384,  // The size of the task stack specified as the number of * bytes.
      NULL,  // Pointer that will be used as the parameter for the task * being created.
      0,     // Priority of the task.
      NULL,  // Task handler.
      0);    // Core where the task should run.    
  M5.Lcd.setTextColor(WHITE, BLACK);
  Serial.begin(9600);
}

void loop() {
  M5.update(); 
  if(isReset){
    if(drawBtns && (isSetCascadeType || !isSetMode)) drawOtherSetBtns();
    if(!isSetMode) displaySettingMode(mode);
    if(!isSetCascadeType && isSetMode && drawBtns) drawSetCascadeBtns();
    if(isSetCascadeType && !isSetTankAHeight) displaySettingValue(tankA.height, "m");
    if(isSetTankAHeight && !isSetTankAWidth) displaySettingValue(tankA.width, "m");
    if(isSetTankAWidth && !isSetTankALength) displaySettingValue(tankA.length, "m");
    if(isSetTankALength && !isSetTankAWaterHeight) displaySettingValueWithMorePrecision(tankA.waterHeight, "m");
    if(isSetTankAWaterHeight && !isSetTankAWaterSupply) displaySettingValueWithMorePrecision(tankA.waterSupply, "m * m * m / s");
    if(isSetTankAWaterSupply && !isSetTankAWaterDrainSurface) displaySettingValueWithMorePrecision(tankA.waterDrainSurface, "m * m");
    if(isSetTankAWaterDrainSurface && !isSetTankBHeight) displaySettingValue(tankB.height, "m");
    if(isSetTankBHeight && !isSetTankBWidth) displaySettingValue(tankB.width, "m");
    if(isSetTankBWidth && !isSetTankBLength) displaySettingValue(tankB.length, "m");
    if(isSetTankBLength && !isSetTankBWaterHeight) displaySettingValueWithMorePrecision(tankB.waterHeight, "m");
    if(isSetTankBWaterHeight && !isSetTankBWaterSupply) displaySettingValueWithMorePrecision(tankB.waterSupply, "m * m * m / s");
    if(isSetTankBWaterSupply && !isSetTankBWaterDrainSurface) displaySettingValueWithMorePrecision(tankB.waterDrainSurface, "m * m");
    if(isSetTankBWaterDrainSurface && !isSetTankCHeight) displaySettingValue(tankC.height, "m");
    if(isSetTankCHeight && !isSetTankCWidth) displaySettingValue(tankC.width, "m");
    if(isSetTankCWidth && !isSetTankCLength) displaySettingValue(tankC.length, "m");
    if(isSetTankCLength && !isSetTankCWaterHeight) displaySettingValueWithMorePrecision(tankC.waterHeight, "m");
    if(isSetTankCWaterHeight && !isSetTankCWaterSupply) displaySettingValueWithMorePrecision(tankC.waterSupply, "m * m * m / s");
    if(isSetTankCWaterSupply && !isSetTankCWaterDrainSurface) displaySettingValueWithMorePrecision(tankC.waterDrainSurface, "m * m");
  }else{    
    if(drawBtns) drawBtnsFun(); 
    if(drawRect) drawRectFun();

    //for changeing current tank     
    if(M5.BtnA.wasPressed() && currentSetTank != "zbiornik A"){
      currentSetTank="zbiornik A";
      M5.Lcd.fillRect(0,0,200,35,BLACK);         
      drawBtns = true; 
      drawTank = true;  
    } 
    if(M5.BtnB.wasPressed() && currentSetTank != "zbiornik B"){
      currentSetTank="zbiornik B"; 
      M5.Lcd.fillRect(0,0,200,35,BLACK); 
      drawBtns = true;  
      drawTank = true;  
    }
    if(M5.BtnC.wasPressed() && currentSetTank != "zbiornik C"){
      currentSetTank="zbiornik C";
      M5.Lcd.fillRect(0,0,200,35,BLACK);
      drawBtns = true; 
      drawTank = true;     
    }
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0,30);    
    M5.Lcd.printf("%s \n",currentSetTank.c_str());       
    displayCurrentProperty();
    if(page == 1){
      displayTank();
      displayWaterLevel();
      displayAlarm(); 
      displayDiodeSenors();
    }  
    carryOnPresetTask(); 
  }
}
