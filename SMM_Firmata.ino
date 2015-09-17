const int START = 128;
const int DIGI_READ = 0;
const int DIGI_WRITE = 32;	//pins 2-13
const int ANA_READ = 64;
const int DIGI_WATCH_2 = 72; //pins 14-19
const int ANA_REPORT = 80;
const int ANA_WRITE = 96;		//pins 3,5,6,9,10,11
const int DIGI_WATCH = 112;	//pins 2-13

void analogReport(int pin,int val){
  Serial.write(START+ANA_READ+(pin<<3)+(val>>7));
  Serial.write(val&127);
  Serial.println("");
}

void digitalReport(int pin,int val){
  Serial.write(START+DIGI_READ+(pin<<1)+((val)?1:0));
  Serial.println("");
}
 
class analogPin {
public:
  int pin;
  int interval;
  long timer;
  bool reporting;
  analogPin(){
    
  }
  analogPin(int p){
    pin=p;
    timer=0;
    interval=1000;
    reporting=false;
  }
  void setInterval(int i){
    interval=i;
    reporting=true;
  }
  void clearInterval(){
    reporting=false;
  }
  void idle(){
    if(timer<millis()&&reporting){
      timer=millis()+interval;
      analogReport(pin,analogRead(pin));
    }
  }
};

String inputString = "";         // a string to hold incoming data
const int length = 10;
unsigned char input[length];
int pntr =0;
boolean stringComplete = false;  // whether the string is complete
analogPin * aPins[6];

void setup() {
  // initialize serial:
  Serial.begin(115200);
  // reserve 200 bytes for the inputString:
  inputString.reserve(255);
  DDRD = DDRD | B00000000;
  DDRB = DDRB | B00000000;
  for(int i=0; i<6; i++){
    aPins[i] = new analogPin(i);
  } 
}

void analogRequest(int pin){
  analogReport(pin,analogRead(pin));
}

void digitalRequest(int pin){
  digitalReport(pin,digitalRead(pin));
}

void digitalPush(int pin,int val){
  pinMode(pin,OUTPUT);
  digitalWrite(pin,val);
}

int awPins[6] = {3,5,6,9,10,11};

void analogPush(int pin, int val){
  analogWrite(awPins[pin],val);
}

void setupReport(int pin,int ival){
  if(ival) aPins[pin]->setInterval(ival);
  else if(ival==0) aPins[pin]->clearInterval();
  
}

char oldPINB =B00000000;
char oldPIND =B00000000;
char oldPINC =B00000000;
char watched[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void watchPort(char &oldP,char newP,int aug){
  char xorP = oldP^newP;
  if(xorP){
    for(int i=0; i<8; i++){
      if(bitRead(xorP,i)&&watched[i+aug]){
        digitalReport(i+aug,bitRead(newP,i));
      }
    }
    oldP=newP;
  }
}

void watchInputs(){
  char newPINB = (PINB & (B11111111^DDRB) & B00111111);
  char newPIND = (PIND & (B11111111^DDRD) & B11111100);
  
  char newPINC = (PINC & (B11111111^DDRC) & B00111100);
  
  watchPort(oldPINB,newPINB,8);
  watchPort(oldPIND,newPIND,0);
  watchPort(oldPINC,newPINC,14);
}

void setWatch(int pin){
  pinMode(pin,INPUT);
  watched[pin]=1;
}

long last =0;

void loop() {
  // print the string when a newline arrives:
  if (stringComplete&&pntr) {
    for(int i=0; i<pntr; i++){
      unsigned char cur = input[i];
      if(cur>=192){
        if(cur==195) cur = input[++i]+64;
        else cur = input[++i];
      }
      //Serial.println((int)cur);
      switch(cur&0b11100000){
        case (START+DIGI_READ):
            digitalRequest(cur&31);
          break;
        case (START+DIGI_WRITE):
            digitalPush((cur&30)>>1,cur&1);
          break;
        case (START+ANA_READ):
          if(cur&16){ //indicates ANA_REPORT
            int nex = input[++i];
            setupReport((cur>>1)&7,2*(((cur<<7)&128)+nex&127));
          }
          else if(cur&8){  //indicates DIGI_WATCH_2
            setWatch((cur&7)+13);
          }
          else analogRequest(cur&7);
          break;
        case (START+ANA_WRITE):
          if(cur&16){    // indicates DIGI_WATCH
            setWatch(cur&15);
          }
          else {
            int nex = input[++i];
            analogPush((cur>>1)&7,((cur<<7)&128)+nex&127);
          }
          break;
      }
    }
      
    // clear the string:
    pntr = 0;
    stringComplete = false;
  }
  watchInputs();
  for(int i=0; i<6; i++){
    aPins[i]->idle();
  } 
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
 int charRemaining = 0;
 bool startBytes =true;
 
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    int inChar = Serial.read(); 
    //Serial.println((unsigned int)(inChar));
     // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '|'||inChar == '\n') stringComplete = true;
    else // add it to the inputString:
      //inputString+=inChar;
      input[pntr++] = inChar;
  }
}


