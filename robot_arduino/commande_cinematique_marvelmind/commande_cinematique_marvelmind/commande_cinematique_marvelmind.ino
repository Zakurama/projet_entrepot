#include "MeShield.h"
#include <Wire.h>

#define PWM_MAX 255            // max value of duty cycle that can be sent
#define V_MAX 12.0             // max voltage that can be sent through the H-bridge
#define V2PWM PWM_MAX / V_MAX 
#define DT 5. // step time in ms
#define VMAX 200

#define RAYON_ROUE 29
#define MAX_WAYPOINTS 10  

// Seuil de distance pour considérer qu'on a atteint le waypoint
#define SEUIL 200.0 // Par exemple, 5 cm de seuil

//-----------------------------------------------------------------//
// Objects
//-----------------------------------------------------------------//
enum Etat {
    RECEPTION_WAYPOINTS,
    INITIALISATION_TRAJ,
    EXECUTION_TRAJ,
    ENVOI_FIN,
};

MeGyro gyro;

struct Point{
  float x; //mm
  float y;
};

struct Point_marvelmind{
  int x; //mm
  int y;
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

  float vx; 
  float vy;

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

// Variable pour suivre l'état actuel
Etat etat_actuel = RECEPTION_WAYPOINTS;
Point_marvelmind marvel_pos = {.x = 0, .y = 0}; 
struct Trajectoire trajectoire = {.Kx = 1, .Ky = 1,.waypoints = {{0,0}}, .nb_waypoints = 0, .current_wp = 0,.x0=0,.Vx0 = 0,.xf = 0,.Vxf = 0,.y0=0,.Vy0 = 0,.yf = 0,.Vyf = 0,.t0 = 0, .tf = 10};
struct Robot robot = {.longueur=126, .largeur=84,.KM = 50., .pos = {0}, .theta=0, 0.};
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

  robot.vx = (cos(robot.theta)*robot.v-robot.w*robot.longueur*sin(robot.theta));
  robot.vy = (sin(robot.theta)*robot.v+robot.w*robot.longueur*cos(robot.theta));
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
  robot.vr = constrain(cos(robot.theta)*vitesse_x_ref + sin(robot.theta)*vitesse_y_ref, -VMAX, VMAX);
  robot.wr = (-sin(robot.theta)*vitesse_x_ref + cos(robot.theta)*vitesse_y_ref)/robot.largeur;
}

//-----------------------------------------------------------------//
// Trajectoire Functions
//-----------------------------------------------------------------//

// return true si le robot à atteint le dernier waypoint qu'il doit atteindre et false sinon
bool trajectoire_update(Trajectoire &trajectoire, Robot &robot, Point_marvelmind marvel_pos){

  int current_wp = trajectoire.current_wp;
  trajectoire.xf = trajectoire.waypoints[current_wp].x;
  trajectoire.yf = trajectoire.waypoints[current_wp].y;

  float t = millis()*0.001;

  // on n'interpole que jusqu'au point final
  if (t < trajectoire.tf){
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
  }
  else {
    trajectoire.xr = trajectoire.waypoints[current_wp].x; 
    trajectoire.dxr =0; 

    trajectoire.yr = trajectoire.waypoints[current_wp].y;
    trajectoire.dyr =0; 
  }

  float distance = sqrt(pow((float)marvel_pos.x - trajectoire.xf, 2) + pow((float)marvel_pos.y - trajectoire.yf, 2));

  // Serial.println("Distance : " + String(distance));
  // Serial.println("current wp : " + String(trajectoire.current_wp));

  if ((distance < SEUIL) && (trajectoire.current_wp < trajectoire.nb_waypoints-1)) {
    // Passer au waypoint suivant, mais seulement si ce n'est pas le dernier waypoint
    trajectoire.current_wp++;
    current_wp = trajectoire.current_wp;
    trajectoire.x0 = marvel_pos.x;
    trajectoire.y0 = marvel_pos.y;
    trajectoire.Vx0 = trajectoire.dxr; 
    trajectoire.Vy0 = trajectoire.dyr; 
    trajectoire.xf = trajectoire.waypoints[current_wp].x;
    trajectoire.yf = trajectoire.waypoints[current_wp].y;
    trajectoire.t0 += t;
    trajectoire.tf = t + trajectoire.tf;
    return false;
  }
  else if ((distance < SEUIL) && (trajectoire.current_wp == trajectoire.nb_waypoints-1)){
    return true;
  }
}

void init_trajectoire(Trajectoire &trajectoire, Point_marvelmind marvel_pos, Robot &robot){
  
  trajectoire.x0 = marvel_pos.x; 
  trajectoire.y0 = marvel_pos.y; 
  robot.pos.x = marvel_pos.x; 
  robot.pos.y = marvel_pos.y; 
  int nb_waypoints = trajectoire.nb_waypoints; 
  for( int i=0 ; i < nb_waypoints  ; i++)
  {
    trajectoire.waypoints[i].x += marvel_pos.x;
    trajectoire.waypoints[i].y += marvel_pos.y;
    /*
    Serial.println("x0 : " + String(trajectoire.x0)); 
    Serial.println("y0 : " + String(trajectoire.y0)); 
    Serial.println("waypoints x : " + String(trajectoire.waypoints[i].x)); 
    Serial.println("waypoints y : " +  String(trajectoire.waypoints[i].y)); 
    */
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
// Marvelmind Initialisation
//-----------------------------------------------------------------//


char buffer[20]; 

void marvelmindsetup (Point_marvelmind *marvel_pos){
  
  Serial2.write("p\n"); 
  int index = 0; 
  int parasiteCount = 3;  // Nombre de caractères à ignorer p\n\0
  while (1)
  {    
    if (Serial2.available()) { // Vérifie si des données sont disponibles sur Serial2
      char c = Serial2.read();  
      // Ignorer les 3 premiers caractères parasites
      if (parasiteCount > 0) { 
          parasiteCount--;  
          continue;  
      }
      //Serial.print(c);            // Affiche ce caractère sur le Serial Monitor
      buffer[index++]=c; 
    
      if (c == '\n' || index >= 19) break;
    }
  }
  buffer[index]='\0';
  
  if (sscanf(buffer, "x:%d,y:%d", &(marvel_pos->x), &(marvel_pos->y)) == 2){
    Serial.println("Parsing réussi !");
    Serial.print("x_marvel = ");
    Serial.println(marvel_pos->x);
    Serial.print("y_marvel = ");
    Serial.println(marvel_pos->y); 
  }

}

// Return 0 if able to read positions and put them in marvel_pos
// Return -1 otherwise
int readmarvelmind (Point_marvelmind *marvel_pos){
   
  Serial2.write("p\n"); 
  int index = 0; 
  strcpy(buffer, ""); 
  int parasiteCount = 3;  // Nombre de caractères à ignorer p\n\0
  while (1)
  {    
    if (Serial2.available()) { // Vérifie si des données sont disponibles sur Serial2
      char c = Serial2.read();  
      // Ignorer les 3 premiers caractères parasites
      if (parasiteCount > 0) { 
          parasiteCount--;  
          continue;  
      }
      //Serial.print(c);            // Affiche ce caractère sur le Serial Monitor
      buffer[index++]=c; 
    
      if (c == '\n' || index >= 19) break;
    }
  }

  buffer[index]='\0';
  Serial.println(buffer); 
  int x, y;

  if (sscanf(buffer, "x:%d,y:%d", &x, &y) == 2){
    marvel_pos->x = x;
    marvel_pos->y = y;
    return 0;
  }
  return -1;
}

//-----------------------------------------------------------------//
// Rotation
//-----------------------------------------------------------------//


void rotation_trigo(float voltage, struct Motor &right_motor, struct Motor &left_motor){
  motor_setVoltage(voltage, right_motor);
  motor_setVoltage(-voltage, left_motor);
  motor_applyVoltage(right_motor); motor_applyVoltage(left_motor);
}

void rotation_anti_trigo(float voltage, struct Motor &right_motor, struct Motor &left_motor){
  motor_setVoltage(-voltage, right_motor);
  motor_setVoltage(voltage, left_motor);
  motor_applyVoltage(right_motor); motor_applyVoltage(left_motor);
}

void rotation(double angle, struct Motor &right_motor, struct Motor &left_motor) {
    bool trigo = true;
    int precision_angle = 5; // en °
    const double max_voltage = 8.0;
    const double min_voltage = 2.0;

    if (fmod(angle + 180, 360) < 180) {
      trigo = false;
    }

    gyro.update();
    double theta0 = gyro.getAngleZ(); // -180 < theta < 180
    double angle_final = fmod(angle + theta0 + 180, 360) - 180;

    // Définir la tension initiale des moteurs
    double voltage = max_voltage;
    if (trigo) {
        rotation_trigo(voltage, right_motor, left_motor);
    } else {
        rotation_anti_trigo(voltage, right_motor, left_motor);
    }

    while (true) {
        gyro.update();
        double current_angle = gyro.getAngleZ();
        Serial.println("current angle: " + String(current_angle));
        double error = angle_final - current_angle;

        // Si l'erreur est dans la précision, arrêter la rotation
        if (abs(error) < precision_angle) {
            break;
        }

        // Si on dépasse l'angle cible, inverser la rotation
        if ((trigo && error < 0) || (!trigo && error > 0)) {
            trigo = !trigo;
            voltage = voltage / 2;

            voltage = max(voltage, min_voltage);


            if (trigo) {
              rotation_trigo(voltage, right_motor, left_motor);
            } else {
              rotation_anti_trigo(voltage, right_motor, left_motor);
            }
        }

        delay(10);
    }

    // Arrêter les moteurs
    motor_setVoltage(0.0, right_motor);
    motor_setVoltage(0.0, left_motor);
    motor_applyVoltage(right_motor); motor_applyVoltage(left_motor);
}

double compute_angle(Point point1, Point point2) {
    return atan2(point2.y - point1.y, point2.x - point1.x) * 180.0 / M_PI; // Convert radians to degrees
}

double get_offset_gyroscope_angle(struct Motor &right_motor, struct Motor &left_motor){
  // mesurer la position à l'aide de marvelmind
  Point_marvelmind point_initial; // mesurer la position à l'aide de marvelmind
  readmarvelmind(&point_initial);

  motor_setVoltage(6, right_motor);
  motor_setVoltage(6, left_motor);
  motor_applyVoltage(motor_D); motor_applyVoltage(motor_G);
  delay(750);

  Point_marvelmind point_final; // mesurer la nouvelle position à l'aide de marvelmind
  readmarvelmind(&point_final);

  Point point1 = {.x = ((float)point_initial.x), .y = ((float)point_initial.y)};
  Point point2 = {.x = ((float)point_final.x), .y = ((float)point_final.y)};

  return compute_angle(point1, point2);
}


//-----------------------------------------------------------------//
// Waypoints Initialisation
//-----------------------------------------------------------------//

void resetTrajectoire(Trajectoire &trajectoire) {
    trajectoire.nb_waypoints = 0;  // On remet le compteur à 0

    for (int i = 0; i < MAX_WAYPOINTS; i++) {
        trajectoire.waypoints[i] = {0, 0}; // On réinitialise chaque waypoint
    }
}
 
void readwaypoints(struct Trajectoire &trajectoire) {
    char buffer_waypoints[50];

    Serial2.write("w\n");
    int index = 0;
    int parasiteCount = 3;  // Ignorer w\n\0

    while (1) {    
        if (Serial2.available()) { 
            char c = Serial2.read();  
            if (parasiteCount > 0) { 
                parasiteCount--;  
                continue;  
            }
            buffer_waypoints[index++] = c; 
            if (c == '\n' || index >= 49) break;
        }
    }

    buffer_waypoints[index] = '\0'; // Terminaison de la chaîne
    Serial.println(buffer_waypoints); 

    resetTrajectoire(trajectoire);  

    char *token = strtok(buffer_waypoints, ";");
    while (token != NULL) {
        int x, y;
        if (sscanf(token, "%d,%d", &x, &y) == 2) {
            trajectoire.waypoints[trajectoire.nb_waypoints] = {x, y};
            trajectoire.nb_waypoints++;
        }
        token = strtok(NULL, ";");
    }

    Serial.println("Nb waypoints : " + String(trajectoire.nb_waypoints)); 
}


//-----------------------------------------------------------------//
// Arduino Functions
//-----------------------------------------------------------------//
void setup() {
  motor_config(motor_D);
  motor_config(motor_G);
  Serial.begin(115200);
  Serial2.begin(115200); 

  marvelmindsetup(&marvel_pos); 
  //init_trajectoire(trajectoire, marvel_pos, robot); 
  
  attachInterrupt(digitalPinToInterrupt(motor_D.VA), onRisingEdge_MD, RISING);
  attachInterrupt(digitalPinToInterrupt(motor_G.VA), onRisingEdge_MG, RISING);
}


void loop() { 

  switch (etat_actuel) {
    case RECEPTION_WAYPOINTS:
        readwaypoints(trajectoire);
        etat_actuel = INITIALISATION_TRAJ;
        break;

    case INITIALISATION_TRAJ:
        long t = millis();
        trajectoire.t0 = t*0.001 ; 
        trajectoire.tf += t*0.001;
        init_trajectoire(trajectoire, marvel_pos, robot); 
        etat_actuel = EXECUTION_TRAJ;
        break;

    case EXECUTION_TRAJ:
        t = millis();
        bool parcours_fini = false;
        readmarvelmind(&marvel_pos);
        parcours_fini = trajectoire_update(trajectoire, robot, marvel_pos);
        //robot_updateVrWr(robot,trajectoire.dxr + trajectoire.Kx*(trajectoire.xr-marvel_pos.x),trajectoire.dyr + trajectoire.Ky*(trajectoire.yr-marvel_pos.y));
        robot_updateVrWr(robot,robot.vx + trajectoire.Kx*(trajectoire.xr-marvel_pos.x), trajectoire.Ky*(robot.vy + trajectoire.yr-marvel_pos.y));
        robot_updateVDrVGr(robot,robot.vr,robot.wr);
        robot_updateVoltage(robot,robot.vitesse_d_r,robot.vitesse_g_r);
        motor_setVoltage(robot.voltage_d, motor_D);
        motor_setVoltage(robot.voltage_g, motor_G);
        motor_updateSpeed(motor_D);
        motor_updateSpeed(motor_G);
        robot_updatePos(robot, motor_D,motor_G);

        motor_applyVoltage(motor_D);
        motor_applyVoltage(motor_G);
        if (parcours_fini and t > trajectoire.t0 + 10000 * trajectoire.nb_waypoints){
          
          motor_setVoltage(0., motor_D);
          motor_setVoltage(0., motor_G);
          etat_actuel = ENVOI_FIN;
        }
        
        break;
        
      case ENVOI_FIN:
        Serial2.write("f\n"); 
        etat_actuel = RECEPTION_WAYPOINTS;
        break; 

  }

}
