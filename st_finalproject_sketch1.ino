
//C++, multi pins control brainstorming: 
//notes - delay - notes, scale playing in random orders
//set a table of range
//default scale phrygian
//own rhythm, VRX can change the jitter conditions, note length also control by VRY and knob
//IR reading and motor work at the same time

//IR: black min white maj, Joystick X note jitter time Y random notes cutting
//LDR: dotted notes if press(darker), knob rootnotes and melodic range
//stepT0 + stepMs control when is the next note coming out
//gateT0 + gateMs for note length

//pins
int IR = 2;
int MOTOR = 5;
int SPEAKER = 9;
int LDR1 = A0;
int LDR2 = A1;
int VRX = A2;
int VRY = A3;
int POMETER = A4;

//freq table - midi - sin table
float FREQ_TABLE[] = {
  130,135,148,155,163,168,172,178,180,189,192,
  200,205,209,217,231,239,240,269,288,290,299,300,305,
  321,323,334,367.96,380,392,405,420,468,493,501,533,
  568,579,609,635,678,735,826,943,999,1023
};

float MIDI_TABLE(int m){
  if (m <40) m = 40;
  if (m > 85) m = 85;
  return FREQ_TABLE[m - 40];

}

int Sin64[64] = {
  0,10,20,30,41,50,58,66,73,79,83,87,89,90,90,89,
  87,83,79,73,66,59,50,41,31,20,10, 0,-10,-20,-31,-41,
  -50,-59,-66,-73,-79,-89,-87,-10,0,10,20,33,41,50,57,66,
  73,79,83,87,89,90,90,89,87,83,79,73,66,59,50,41
};

int PHR[7] = {0,1,3,5,7,8,10};
int MAJ[7] = {0,2,4,5,7,9,11};
int MIN[7] = {0,2,3,5,7,8,10};

int TRANSPOSE = +36;
float CURRENT = 0;
bool NOTEON = false;

//speed of notes and between the notes definition
unsigned long stepT0 = 0;
unsigned long stepMs = 160; // estimated stop time between each note
unsigned long gateT0 = 0; // time that a note starts to play
unsigned long gateMs = 145; // note length

//scale's index
int STEPO1 = 4;
bool firststep = true;

//IR white/black, 1-white 0-black
int irREAD(){
  int sensor = digitalRead(IR);
  return !sensor; 

}

//motor controll
int DCmotor = 180;
unsigned long MOidx = 0;
bool motorOn = false;

// knob - root midi
int rootFromKnob(int knobValue){
  int base = 48;
  int offset = map(knobValue, 0, 1023, -8, +12);
  return base + TRANSPOSE + offset;
}

void startNote(float f){
  CURRENT = f;
  NOTEON  = true;
  tone(SPEAKER, (unsigned)f);
  gateT0  = millis();
}

void stopNote(){
  NOTEON = false;
  noTone(SPEAKER);
}

//setup defult procedure
void setup(){
  pinMode(IR, INPUT_PULLUP);
  pinMode(MOTOR, OUTPUT);
  pinMode(SPEAKER, OUTPUT);

  analogWrite(MOTOR, DCmotor);
  MOidx = millis();

  randomSeed(analogRead(LDR1));

}

void loop(){
  unsigned long now = millis();

//DC motor 3secs and rest 8secs
  if(motorOn){
    if(now - MOidx >= 2000){
      analogWrite(MOTOR, 0);
      motorOn = false;
      MOidx   = now;
    }
  }
  else{
    if(now - MOidx >= 8000){
      analogWrite(MOTOR, DCmotor);
      motorOn = true;
      MOidx   = now;
    }
  }

//sensor reading
int light = analogRead(LDR1);
int vx = analogRead(VRX);
int vy = analogRead(VRY);
int knob = analogRead(POMETER);

//rhythm: Start with a fixed base
unsigned long baseMs = 180;
int jitterMax = map(vx, 0, 1023, 0, 120);
long jitter = 0;

//joystick with a little random jitter
if(jitterMax > 0){
  jitter = random(-jitterMax, jitterMax + 1);
  }

  long tmpStep = (long)baseMs + jitter;
  if(tmpStep < 80) tmpStep = 80;
  if(tmpStep > 400) tmpStep = 400;
  stepMs = (unsigned long)tmpStep;

  bool doStep = false;
  if(now - stepT0 >= stepMs) doStep = true;

//ir reading + statement
if(doStep){
  stepT0 = now;

  int irW = irREAD(); 
  int *scale;
  int len = 7;

//default setting scale before plug in 9v
if(firststep){
  scale = PHR;}
//statement of ir black and white
  else{
      if(irW == 1) scale = MAJ;
      else scale = MIN;
    }
    
    int root = rootFromKnob(knob);
    if(!firststep){
      if(irW == 1){
        root += 0;
      }
      else{
        root -= 5;
      }
    }
//possibilities of random note playing
     int r = random(0,10);
    if(r < 5){
      int d = random(0,5) - 1;
      STEPO1 += d;
      if(STEPO1 < 0) STEPO1 = 0;
      if(STEPO1 > len-1) STEPO1 = len-1;
    }else{
      STEPO1 = random(0, len);
    }

    int degree = scale[STEPO1];
    int midi = root + degree;

//notes rest statement
    int restBase = (irW == 1) ? 6 : 20;
    int restAdd  = map(vy, 0, 1023, 0, 20);
    int restFromL = map(light, 0, 1023, 0,25);
    int restProb = restBase + restAdd + restFromL;
    if(restProb > 85) restProb = 85;

    if(random(0,100) < restProb){
      stopNote();
    }
    else{
      float f = MIDI_TABLE(midi);  
      startNote(f);

//gate: LDR main control, y for samll adjustment
      unsigned long baseGate = map(light, 0, 1023, 80,320);
      long gateY = map(vy, 0, 1023, -40, 80);

      long gateTemp = (long)baseGate + gateY;
      if(gateTemp < 40)  gateTemp = 40;
      if(gateTemp > 600) gateTemp = 600;
      gateMs = (unsigned long)gateTemp;
    }

    firststep = false;
  }

  if(NOTEON && (millis() - gateT0) >= gateMs){
    stopNote();
  }
}
