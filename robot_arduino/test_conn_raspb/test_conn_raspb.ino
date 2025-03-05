int x_marvel, y_marvel;
char buffer[20]; 

void setup() {
  Serial.begin(115200);  // Initialisation du port série principal
  Serial2.begin(115200);   // Initialisation du port Serial2 à 9600 bauds (ajuste selon ton module)
}

void loop() {
     
    int index = 0; 
    while (1)
    {    
      if (Serial2.available()) { // Vérifie si des données sont disponibles sur Serial2
        char c = Serial2.read();    // Lit un caractère depuis Serial2 (Hello World)
        //Serial.print(c);            // Affiche ce caractère sur le Serial Monitor
        buffer[index++]=c; 
      
      if (c == '\n' || index >= 19) break;
      }
    }
    buffer[index]='\0';

    if (sscanf(buffer, "x:%d,y:%d", &x_marvel, &y_marvel) == 2){
      Serial.println("Parsing réussi !");
      Serial.print("x_marvel = ");
      Serial.println(x_marvel);
      Serial.print("y_marvel = ");
      Serial.println(y_marvel); 
    }
  
}
