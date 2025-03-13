#define PWM_MAX 255            // max value of duty cycle that can be sent
#define V_MAX 12.0             // max voltage that can be sent through the H-bridge
#define V2PWM PWM_MAX / V_MAX 
#define DT 5. // step time in ms

#define RAYON_ROUE 29
#define MAX_WAYPOINTS 10  

// Seuil de distance pour considérer qu'on a atteint le waypoint
#define SEUIL 200.0 // Par exemple, 5 cm de seuil

//-----------------------------------------------------------------//
// Objects
//-----------------------------------------------------------------//
struct Point{
  float x; //mm
  float y;
};

struct Motor{
  int VA;
  int VB;
  int I1; 
  int I2;
  int PWM;

  int pulse_par_tour;
  int rapport_reduction;

  String type_motor;
  
  float u;
  float v; 
  long pulse;
  long pulse_precedent;
  long time_last_mesure;
  float last_speed;
};


struct Robot{
  int longueur;
  int largeur;
  float KM;
  
  Point pos;
  float theta;

  float v;
  float w;
  float vr;
  float wr;
  
  long time_last_mesure;
  float voltage_d;
  float voltage_g;
  float vitesse_d_r;
  float vitesse_g_r;
  
};

struct Trajectoire{

  float Kx;
  float Ky;

  Point waypoints[MAX_WAYPOINTS];
  int nb_waypoints;
  int current_wp;

  float x0;
  float Vx0;
  float xf;
  float Vxf;
  
  float y0;
  float Vy0;
  float yf;
  float Vyf;

  float t0;
  float tf;

  float xr;
  float dxr;
  float yr;
  float dyr;
  };

//-----------------------------------------------------------------//
// Build global objects
//-----------------------------------------------------------------//
struct Trajectoire trajectoire = {.Kx = 0.5, .Ky = 0.5,.waypoints = {{500, 0}, {500, 500}, {1000,500}}, .nb_waypoints = 3, .current_wp = 0,.x0=0,.Vx0 = 0,.xf = 0,.Vxf = 0,.y0=0,.Vy0 = 0,.yf = 0,.Vyf = 0,.t0 = 0, .tf = 4};
struct Robot robot = {.longueur=126, .largeur=84,.KM = 50., .pos = {0}, .theta=0,0};
struct Motor motor_G = {.VA=18, .VB=31, .I1=34, .I2=35, .PWM=12,.pulse_par_tour = 8, .rapport_reduction = 46,.type_motor = "Gauche",0};
struct Motor motor_D = {.VA=19, .VB=38, .I1=37, .I2=36, .PWM=8,.pulse_par_tour = 8, .rapport_reduction = 46,.type_motor = "Droit",0};

//-----------------------------------------------------------------//
// Motors Functions
//-----------------------------------------------------------------//
void motor_config(const struct Motor &motor){
    pinMode(motor.VA, INPUT_PULLUP);
    pinMode(motor.VB, INPUT_PULLUP);
  
    pinMode(motor.I1, OUTPUT);
    pinMode(motor.I2, OUTPUT);
    pinMode(motor.PWM, OUTPUT);
}

void motor_setVoltage(const float voltage, struct Motor &motor){
  
  if (motor.type_motor == "Gauche"){
      motor.u = -voltage;
  }
  else{
    motor.u = voltage;
  }
  
}

void motor_updateSpeed(struct Motor &motor){

  float time_actuel = millis();
  
  float d_pulse = motor.pulse - motor.pulse_precedent;
  
  float dt = (time_actuel - motor.time_last_mesure);
  
  float nb_tour = d_pulse/motor.pulse_par_tour;
  float distance = RAYON_ROUE*nb_tour*2*PI/(motor.rapport_reduction);

  float vitesse = 1000 * distance / dt; // 1000 pour s

  if (motor.type_motor == "Gauche"){
    motor.last_speed = -vitesse;
  }
  else{
    motor.last_speed = vitesse;
 }
  
  
  motor.pulse_precedent = motor.pulse;
  motor.time_last_mesure = time_actuel;
  }

void motor_applyVoltage(const struct Motor &motor){
    float voltage = motor.u;

    if (abs(voltage) < 1 ) {voltage = 0;}
    if (voltage > 0)
    {
        digitalWrite(motor.I2, LOW);
        digitalWrite(motor.I1, HIGH);
    }
    else
    {
        digitalWrite(motor.I1, LOW);
        digitalWrite(motor.I2, HIGH);
    }
    analogWrite(motor.PWM, constrain(abs(voltage) * V2PWM, 0, V_MAX * V2PWM));
}
   

//-----------------------------------------------------------------//
// Robot Functions
//-----------------------------------------------------------------//

void robot_updatePos(Robot &robot,Motor &motor_D, Motor &motor_G){

  float time_actuel = millis();  
  float dt = (time_actuel - robot.time_last_mesure)*0.001;
  
  robot.v = (motor_D.last_speed + motor_G.last_speed)/2;
  robot.w = (motor_D.last_speed - motor_G.last_speed)/(2*robot.largeur);

  robot.pos.x = robot.pos.x + dt* (cos(robot.theta)*robot.v-robot.w*robot.longueur*sin(robot.theta));
  robot.pos.y = robot.pos.y + dt* (sin(robot.theta)*robot.v+robot.w*robot.longueur*cos(robot.theta));
  robot.theta = robot.theta + dt* robot.w;

  robot.time_last_mesure = time_actuel;
  }

void robot_updateVoltage(Robot &robot,float vitesse_vd_ref,float vitesse_vg_ref){
  robot.voltage_d = 1/robot.KM*vitesse_vd_ref;
  robot.voltage_g = 1/robot.KM*vitesse_vg_ref;
  }

void robot_updateVDrVGr(Robot &robot,float vitesse_v_ref,float vitesse_w_ref){
  robot.vitesse_d_r = vitesse_v_ref + robot.largeur*vitesse_w_ref;
  robot.vitesse_g_r = vitesse_v_ref - robot.largeur*vitesse_w_ref;
  }

void robot_updateVrWr(Robot &robot, float vitesse_x_ref,float vitesse_y_ref){
  robot.vr = cos(robot.theta)*vitesse_x_ref + sin(robot.theta)*vitesse_y_ref;
  robot.wr = (-sin(robot.theta)*vitesse_x_ref + cos(robot.theta)*vitesse_y_ref)/robot.largeur;
}

//-----------------------------------------------------------------//
// Trajectoire Functions
//-----------------------------------------------------------------//

void trajectoire_update(Trajectoire &trajectoire, Robot &robot){

  int current_wp = trajectoire.current_wp; 
  trajectoire.xf = trajectoire.waypoints[current_wp].x; 
  trajectoire.yf = trajectoire.waypoints[current_wp].y; 

  float t = millis()*0.001;

  float a0x = trajectoire.x0;
  float a1x = trajectoire.Vx0*(trajectoire.tf-trajectoire.t0);
  float a2x = (trajectoire.Vxf + trajectoire.Vx0)/(2*(trajectoire.tf-trajectoire.t0)) - a1x; 
  float a3x = trajectoire.xf - a0x - a1x -a2x ;

  float a0y = trajectoire.y0;
  float a1y = trajectoire.Vy0*(trajectoire.tf-trajectoire.t0);
  float a2y = (trajectoire.Vyf + trajectoire.Vy0)/(2*(trajectoire.tf-trajectoire.t0)) - a1y;
  float a3y = -a2y + trajectoire.yf - trajectoire.y0 - a1y;
  
  float lambda = (t-trajectoire.t0)/(trajectoire.tf-trajectoire.t0);


  trajectoire.xr = a0x + a1x*lambda + a2x*lambda*lambda + a3x*lambda*lambda*lambda;
  trajectoire.dxr = 1/(trajectoire.tf-trajectoire.t0)*(a1x + 2*a2x*lambda + 3*a3x*lambda*lambda);

  trajectoire.yr = a0y + a1y*lambda + a2y*lambda*lambda + a3y*lambda*lambda*lambda;
  trajectoire.dyr = 1/(trajectoire.tf-trajectoire.t0)*(a1y + 2*a2y*lambda + 3*a3y*lambda*lambda);


  float distance = sqrt(pow(trajectoire.xr - trajectoire.xf, 2) + pow(trajectoire.yr - trajectoire.yf, 2));

  /*Serial.println("Distance : " + String(distance)); */
  Serial.println("current wp : " + String(trajectoire.current_wp)); 

  if ((distance < SEUIL) && (trajectoire.current_wp != trajectoire.nb_waypoints-1)) {
    // Passer au waypoint suivant, mais seulement si ce n'est pas le dernier waypoint
    if (trajectoire.current_wp < trajectoire.nb_waypoints - 1) {
      trajectoire.current_wp++;
      current_wp = trajectoire.current_wp; 
      trajectoire.x0 = robot.pos.x;  
      trajectoire.y0 = robot.pos.y; 
      trajectoire.Vx0 = trajectoire.dxr; //robot.vr; 
      trajectoire.Vy0 = trajectoire.dyr; //robot.wr; 
      trajectoire.xf = trajectoire.waypoints[current_wp].x; 
      trajectoire.yf = trajectoire.waypoints[current_wp].y; 
      trajectoire.t0 += t; 
      trajectoire.tf = t + trajectoire.tf;
    } 
  }
}

//-----------------------------------------------------------------//
// Sensors Functions
//-----------------------------------------------------------------//
void onRisingEdge_MD(){
  if(digitalRead(motor_D.VB)){
    motor_D.pulse++;  
  }else{
    motor_D.pulse--;
  }
}


//-----------------------------------------------------------------//
void onRisingEdge_MG(){
  if(digitalRead(motor_G.VB)){
    motor_G.pulse++;  
  }else{
    motor_G.pulse--;
  }
}




//-----------------------------------------------------------------//
// Arduino Functions
//-----------------------------------------------------------------//
void setup() {
  motor_config(motor_D);
  motor_config(motor_G);
  Serial.begin(115200);

  attachInterrupt(digitalPinToInterrupt(motor_D.VA), onRisingEdge_MD, RISING);
  attachInterrupt(digitalPinToInterrupt(motor_G.VA), onRisingEdge_MG, RISING);
}


void loop() { 

  long t = millis();
  if (t<= trajectoire.tf* 1000 and t > 0) {

    trajectoire_update(trajectoire, robot);
    robot_updateVrWr(robot,trajectoire.dxr + trajectoire.Kx*(trajectoire.xr-robot.pos.x),trajectoire.dyr + trajectoire.Ky*(trajectoire.yr-robot.pos.y));
    robot_updateVDrVGr(robot,robot.vr,robot.wr);
    robot_updateVoltage(robot,robot.vitesse_d_r,robot.vitesse_g_r);
    motor_setVoltage(robot.voltage_d, motor_D);
    motor_setVoltage(robot.voltage_g, motor_G);
    motor_updateSpeed(motor_D);
    motor_updateSpeed(motor_G);
    robot_updatePos(robot, motor_D,motor_G);
    Serial.println("Position robot : ");
    Serial.println("x : " + String(robot.pos.x));
    Serial.println("y : " + String(robot.pos.y));
    /*Serial.println("theta : " + String(robot.theta));
    Serial.println("v : " + String(robot.v));
    Serial.println("w : " + String(robot.w));*/
    /*Serial.println("motor_G voltage : "+ String(robot.voltage_g)); 
    Serial.println("motor_D voltage : "+ String(robot.voltage_d)); 
    Serial.print("motor_G_voltage:");
    Serial.print(robot.voltage_g);
    Serial.print(",");
    Serial.print("motor_D_voltage:");
    Serial.println(robot.voltage_d);*/
    
  }
  else {
    motor_setVoltage(0., motor_D);
    motor_setVoltage(0., motor_G);
  }

  motor_applyVoltage(motor_D); motor_applyVoltage(motor_G);
}
