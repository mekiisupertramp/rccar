                                      /*===========================================================================*=
   Dipl�me voiture RC  /  Mai-Juin 2015  /  Blazevic Mehmed
  =============================================================================
   Descriptif: 
   
   Ce programme a pour but de faire une communication avec un 
   smartphone fonctionnant sous la plateforme Android, en passant par un module
   communiquant en Bluetooth. Le smartphone envoie des donn�es au module 
   Bluetooth, puis celui-ci les achemine au F320 en utilisant une interface UART.

   En fonction des informations re�ues par le smartphone, le microcontr�leur devra
   piloter un moteur et un servomoteur. Ce qui permetra � la voiture RC de se
   d�placer.

   Enfin, le programme interpr�te des donn�es re�ues par un acc�l�rom�tre et 
   en fonction de ces donn�es, le micro devra faire un syst�me antipatinage. 
   Ceci permetra aux roues de ne pas patiner lors d'acc�l�rations trop brutes.

   (Enregistrement des donn�es sur une carte SD et communication avec RTC)


   R�vision: 1.0.0

   Entreprise: Centre de Formation Professionnel Technique

   Professeur : Thierry Forestier
 

=*===========================================================================*/

#include <c8051f320.h>           // registres c8051f320
#include <STRING.H>              // gestion de chaines de caract�res

// ==== FONCTIONS PROTOTYPES===================================================

void ClockInit  ();       // init. clock syst�me
void PortInit   ();       // init. config des ports
void IRQInit    ();       // init. interruption externe
void Timer1Init ();       // init. du timer 1 pour la communication s�rie
void Timer2Init ();		  // init. du timer 2 pour base de temps
void Timer3Init ();       // init. config du timer 3 pour Timeout
void UARTInit   ();       // init. la transmission s�rie
void PCAInit    ();       // init. mode fonctionnement des PCA
void ADInit     ();       // init. mode de fonctionnement du convertisseur AD

unsigned char DecToCarHexa (unsigned char val);     // conv. d�cimal to hexa
unsigned char CarHexaToDec (unsigned char carHexa); // conv. hexa to d�cimal


// fonction d'�mission de l'UART
void EmissionUART(char* ptrText); 
// traitement de la trame re�u
void traitement(unsigned char noCMD,char* ptrIn, char* ptrOut,unsigned char val); 
// contr�le la validit� d'une trame : start, stop et checksum corrects
bit controleTrame (char* ptrTxtIn, unsigned char nbCar);
// permet de d�terminer si la commande re�ue est connue
unsigned char determineCommande (char* ptrTxtIn);
// pilotage marche du moteur
void commandeMoteur (unsigned char valCmd);
// pilotage du servomoteur
void commandeServomoteur(unsigned char valCmd);
// traitement de la valeur de la vitesse
unsigned char traitementVitesse(unsigned int compteur);
// traitement de la valeur de la distance
unsigned char traitementDistance(unsigned char distance);

// ==== CONSTANTES ============================================================

#define LOADVALUE  204     // valeur pour 4.34 us avec un clock � 12 MHz
                           // valeur = 256 - (p�riode d�sir�e * clock)
                           // valeur = 256 - (clock/fr�quence d�sir�e)
                           //        = 256 - (12 MHz/230'400 Hz)
                           //        = 256 - 52
                           //        = 256 - 52
                           //        = 204
                           // baudrate th�orique =  1/((52/12MHz)*2) = 115'384
                           // erreur = 0.16%

#define LOADVALUE2 53536   // valeur pour 1 milliseconde avec un clock � 12MHz
                           // valeur = 65536 - (p�riode d�sir�e * clock)
                           // valeur = 65536 - (clock/fr�quence d�sir�e)
                           //        = 65536 -  12MHz / 1000
                           //        = 65536 - 12000
                           //        = 53536

#define LOADVALUE3   53000 // valeur pour 3.5 ms avec un clock � 12 MHz
                           // valeur = 65536 - (p�riode d�sir�e * clock)
                           //        = 65536 - (clk / fr�quence d�sir�e)
                           //        = 65536 - (12Mhz / 285Hz)
                           //        = 65536 - 42106
                           //        = 23430 -> 23000 pour avoir de la marge
                           // mesure avec oscillo : 3.52 ms -> parfait !

#define BUFFER_MAX   30    // nombre de donn�es maximum

#define UART_CTS 	   0x08  // masque pour CTS sur port 0

#define OFFSET          12000    // offset pour les moteurs
#define OFFSET2			14000	 // offset 2 pour moteur principale
#define PULSE_PAS         130    // pour convertir le pourcentage en pas
#define PULSE_DEMI_PAS	   75	 // pour convertie le pourcentage en pas plus petit
#define PCA_INIT_MOT    18500    // valeur initial pour une pulse de 1.5 ms
#define PCA_INIT_SERVO  17950    // valeur initial pour une pulse de 1.47 ms

// caract�res de commande
#define START1       '*'      // premier caract�re de start de la trame
#define START2       '$'      // seconde caract�re de start de la trame
#define STOP1        '\r'     // premier carac�tre de stop de la trame
#define STOP2        '\n'     // second caract�re de stop de la trame

// identificateurs de trames re�ues 
#define MOTEUR       "CM"     // commande pour moteur                     
#define TOURNER      "CT"     // commande pour tourner           
#define ANTIPAT      "AP"     // commande pour l'antipatinage
#define VITESSE      "VI"     // identificateur pour la vitesse
#define BATTERIE     "NB"     // identificateur pour le niveau de la batterie    
                                                                 
// identificateur acknoledge envoy�                              
#define ACK_AV       "cm"     // r�ponse pour avancer                       
#define ACK_TO       "ct"     // r�ponse pour tourner            
#define ACK_AP       "ap"     // r�ponse pour l'antipatinage 
#define ACK_VIT      "vi"     // acknoledge re�u pour la vitesse
#define ACK_BAT      "nb"     // acknoledge re�u pour le niveau de la batterie    

// commandes
#define COMMANDE_INCONNUE 0xFF   // commande inconnue
#define CMD_MO            0      // commande pour moteur
#define CMD_TO            2      // commande pour tourner
#define CMD_AP            3      // commande pour l'antipatinage
#define REQ_VI            4      // requ�te pour la vitesse
#define REQ_BA            5      // requ�te pour la batterie
#define CMD_CONNEXION	  6		// commande pour connexion Bluetooth
#define CMD_DECONNXION    7      // commande pour la d�connexion Bluetooth

// messages d'erreurs
#define MSG_ERREUR_CTRL        "\r\nERREUR DANS LE CONTROLE \r\n"
#define MSG_ERREUR_CMD         "\r\nERREUR DANS LA COMMANDE \r\n"

// messages de connexion et de d�connexion envoy�s par le module Bluetooth
xdata char trameConnect[]    = {0x02,0x69,0x0C,0x07,0x00};
xdata char trameDisconnect[] = {0x02,0x69,0x11,0x02,0x00};


// ==== VARIABLES GLOBALES ====================================================

xdata char bufferIn[BUFFER_MAX + 1]; // buffer de r�ception + 1 pour z�ro term.
xdata char bufferOut[BUFFER_MAX + 1];// buffer de r�ception + 1 pour z�ro term.

bit strRec = 0;      // signal la r�ception d'une trame
bit sendBusy = 0;    // signal qu'une transmission en cours
bit antipatinage = 0;// bit de commande l'antipatinage (inactif au d�marrage)
bit flagLectureVitesse  = 0;   // flag qui indique qu'on peut lire la vitesse  
bit flagLectureDistance = 0;   // flag qui indque qu'on peut lire la distance
bit flagDistance = 1;          // flag permettant aux moteurs de fonctionner

unsigned char posRec = 0; // position dans le tableau de r�ception
unsigned char nbCaracteres = 0;  // nombre de caract�res re�us dans une trame

unsigned int cptVitesse = 0;     // compteur de vitesse
unsigned int cptTempo500ms = 0;  // compteur pour base de temps 100ms
unsigned char valeurAD;          // la valeure brute du convertisseur



// ==== SBIT ==================================================================

sbit DEBUG = P2^7;   // pin qui va servir de debug pour le programme
sbit UART_RTS = P0^2;   // RTS: permet de d�marrer une communication UART

//sbit UART_CTS = P0^3;   

// ==== MAIN ==================================================================
void main () 
{
   // --- variables locales ---------------------------------------------------
   bit strSend = 0;     // signal une demande d'�mission de trame
   unsigned char noCommande;
   unsigned char vitesseMs;   // vitesse en m�tres / secondes
   unsigned char valDistance; // valeur de la distance en centim�tres
   // -------------------------------------------------------------------------

   PCA0MD &= ~0x40;     // WDTE = 0 (disable watchdog timer)
   ClockInit  ();       // init. clock syst�me
   PortInit   ();       // init. config des ports
   IRQInit    ();       // init. le fonctionnement de l'interruption externe      
   UARTInit   ();       // init. mode de fonctionnement de la COMM. s�rie RS232
   Timer2Init ();		   // init. timer 2 pour base de temps
   Timer3Init ();       // init. config du timer 3
   PCAInit    ();       // init. mode de fonctionnement des PCA
   ADInit     ();       // init. mode fonctionnement convertisseur AD
   
   EA  = 1;             // autorisation g�n�rale des interruptions
   EX0 = 1;             // autorisation de l'interruption externe
   ES0 = 1;             // autorisation interruption UART
   ET2 = 1;             // autorisation du timer 2
   EIE1 |= 0x80;        // autorise interruption timer 3
   EIE1 |= 0x08;        // autorise l'interruption du convertisseur AD
   CR = 1;              // autorisation du PCA

   TR1 = 1;             // start timer 1 : start le g�n�rateur de baudrate  
   TR2 = 1;             // start le timer 2 pour base de temps

   UART_RTS = 0;  // request to send activ�


   while (1)
   {   
      // traitement de la r�ception d'une trame ------------------------------- 
      if(strRec)  // si r�ception d'une trame
      {
         strRec = 0;       // clear flag 

         // contr�le start, checksum et stop
         if(controleTrame(bufferIn,nbCaracteres))
         {
            // d�termine si la commande existe
            noCommande = determineCommande(bufferIn);

            if(noCommande != COMMANDE_INCONNUE)   
            {
               // teste si requ�te ou commande
               if((noCommande == REQ_BA) || (noCommande == REQ_VI))
               {
                  // si requ�te vitesse
                  if(noCommande == REQ_VI) traitement(noCommande,bufferIn,bufferOut,vitesseMs);
               }
               else 
               {
                  // traitement de la trame re�ue
                  traitement(noCommande,bufferIn, bufferOut, 0);
               }
            }
            else
            {
               // la commande n'existe pas !
               strcpy(bufferOut,MSG_ERREUR_CMD);
            }
         }
         else
         {
            // erreur dans start, checksum ou stop
            strcpy(bufferOut,MSG_ERREUR_CTRL);
         }

         strSend = 1;      // demande d'envoi d'une trame (ack ou erreur)                 
      }


      // traitement de la transmission d'une trame ----------------------------
      if(strSend)
      {
         if(!sendBusy)   // si demande d'envoi d'une trame
         {
            EmissionUART(bufferOut);   // envoi de la trame
            sendBusy = 1;              // trame en cours d'envoi
            strSend = 0;               // clear demande d'envoi
         }
      }
      

      // traitement de la lecture de la vitesse -------------------------------
      if(flagLectureVitesse)
      {
         flagLectureVitesse=0;
         vitesseMs = traitementVitesse(cptVitesse);
         cptVitesse = 0;
      }


      // traitement de la lecture de la distance ------------------------------
      if(flagLectureDistance)
      {
         valDistance = traitementDistance(valeurAD);
         flagLectureDistance = 0;
      }

   } // End while (1)
 
} // main =====================================================================

/*---------------------------------------------------------------------------*-
   traitementDistance ()    
  -----------------------------------------------------------------------------
   Descriptif: convertie la valeur du convertisseur AD en distance

   Entr�e    : valeur de le distance brute 			(0..255)
   Sortie    : valeur de la distance en centim�tre			(0..38)
-*---------------------------------------------------------------------------*/
unsigned char traitementDistance (unsigned char distance)
{
   unsigned char distance_cm = 0;
   
   // si un objet se trouve � moins de 15 centim�tre de la voiture
   if(distance > 23)
   {
      flagDistance = 0;
   }    
   else
   {
      flagDistance = 1;   
   }

   return distance_cm;

} // traitementDistance --------------------------------------------------------

/*---------------------------------------------------------------------------*-
   lectureDistance ()    
  -----------------------------------------------------------------------------
   Descriptif: m�morise la valeur du convertisseur AD
   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void lectureDistance () interrupt INTERRUPT_ADC0_EOC
{
 
   // m�morise la valeur sur 8 bits
   valeurAD = ADC0H; 
  
   flagLectureDistance = 1;       // signal une mesure termin�e
   AD0INT = 0;       // clear flag d'interruption

} // lectureDistance ----------------------------------------------------------

/*---------------------------------------------------------------------------*-
   traitementVitesse ()
  -----------------------------------------------------------------------------
   Descriptif: Transforme le nombre de pulse en vitesse 

   Entr�e    : compteur, nombre de pulses sur l'arbre de transmission (0..~560)
   Sortie    : buffer, valeur de le vitesse en m�tres / secondes
-*---------------------------------------------------------------------------*/
unsigned char traitementVitesse (unsigned int compteur) 
{
   double buffer;
   char buffRound;
      
   buffer = compteur / 2.923; // calcule la rotation de la roue avec rapport
   buffer = buffer / 4;       // calcule rotation de la roue (4 coches arbre)
   buffer = buffer * 195.1;   // calcule la distance parcourue en millim�tres
   buffer = buffer / 1000;    // transforme la distance mm en m

   // arrondi la valeur � la demi bonne
   buffRound = buffer * 10;
   buffRound = buffRound % 10;

   if(buffRound > 4)
   {
      buffer = buffer + 0.5;
   }

   return (unsigned char)buffer;
} // traitementVitesse --------------------------------------------------------

/*---------------------------------------------------------------------------*-
   commandeServomoteur ()
  -----------------------------------------------------------------------------
   Descriptif: Pilote le servomoteur

   Entr�e    : valCmd, valeur de commande pour le servomoteur          (0..100) 
   Sortie    : --
-*---------------------------------------------------------------------------*/
void commandeServomoteur (unsigned char valCmd) 
{
   unsigned int valeur;
   
   // si un objet est trop proche de la voiture
   if(flagDistance)
   {
      if((valCmd > 40) && (valCmd < 60))  // point milieu (schmitt)     
      {
         PCA0CPL0 = ~PCA_INIT_SERVO%256;
         PCA0CPH0 = ~PCA_INIT_SERVO/256;    
      }
      else
      {        
		   valeur = OFFSET + ((int)valCmd * PULSE_PAS); 
         PCA0CPL0 = ~valeur%256;
         PCA0CPH0 = ~valeur/256;			 
      }   
   }
   else
   {
      PCA0CPL0 = ~PCA_INIT_SERVO%256;
      PCA0CPH0 = ~PCA_INIT_SERVO/256;     
   }
   
} // commandeServomoteur ------------------------------------------------------

/*---------------------------------------------------------------------------*-
   commandeMoteur ()
  -----------------------------------------------------------------------------
   Descriptif: Pilote le moteur principale en marche avant ou arri�re

   Entr�e    : valCmd, valeur de commande pour le moteur principale    (0..100) 
   Sortie    : --
-*---------------------------------------------------------------------------*/
void commandeMoteur (unsigned char valCmd) 
{
   unsigned int valeur;
   
   // si un objet est trop proche de la voiture
   if(flagDistance)
   {
      if((valCmd > 40) && (valCmd < 60))
      {
         PCA0CPL1 = ~PCA_INIT_MOT%256;
         PCA0CPH1 = ~PCA_INIT_MOT/256;     
      }
      else
      {
       if((valCmd > 59) && (valCmd < 95))
		 {
		 	valeur = OFFSET2 + ((int)valCmd * PULSE_DEMI_PAS); 
        	PCA0CPL1 = ~valeur%256;
         PCA0CPH1 = ~valeur/256;	
		 }
		 else
		 {
		 	valeur = OFFSET + ((int)valCmd * PULSE_PAS); 
         PCA0CPL1 = ~valeur%256;
         PCA0CPH1 = ~valeur/256;	
		 }    
      } 
   }
   else
   {
    //  PCA0CPL1 = ~PCA_INIT_MOT%256;
    //  PCA0CPH1 = ~PCA_INIT_MOT/256;  
      
      PCA0CPL1 = ~15500%256;
      PCA0CPH1 = ~15500/256;  
   }            
} // commandeMoteurAV --------------------------------------------------------

/*---------------------------------------------------------------------------*-
   determineCommande ()
  -----------------------------------------------------------------------------
   Descriptif: D�termine le num�ro de la commande (255 si inconnue)

   Entr�e    : ptrTxtIn, l'adresse de la trame re�ue    
   Sortie    : le num�ro de la commande                                (0..255)
-*---------------------------------------------------------------------------*/
unsigned char determineCommande (char* ptrTxtIn) 
{
   char txtcommande[3];
   
	// isole la commande en chaine de caract�res
	txtcommande[0] = ptrTxtIn [2];
	txtcommande[1] = ptrTxtIn [3];
	txtcommande[2] = 0;
	
	if (strcmp(ptrTxtIn,trameConnect)) {} else return (CMD_CONNEXION);
	if (strcmp (txtcommande,MOTEUR))   {} else return (CMD_MO); 
	if (strcmp (txtcommande,TOURNER))  {} else return (CMD_TO); 
	if (strcmp (txtcommande,ANTIPAT))  {} else return (CMD_AP);
   if (strcmp (txtcommande,VITESSE))  {} else return (REQ_VI);

	return (COMMANDE_INCONNUE);      // retourne le no de commande inconnue
      
} // determineCommande --------------------------------------------------------

/*---------------------------------------------------------------------------*-
   controleTrame ()
  -----------------------------------------------------------------------------
   Descriptif: V�rifie la validit� des champs <START>, <CHECKSUM> et <STOP>

   Entr�e    : ptrTxtIn, l'adresse de la trame re�ue    
               nbPosition, le nombre de caract�res dans la trame        (0..3)
   Sortie    : information de trame valide                (1 si ok et 0 sinon)
-*---------------------------------------------------------------------------*/
bit controleTrame (char* ptrTxtIn, unsigned char nbCar) 
{
   bit tramevalide = 1;
   unsigned char i;
   unsigned char checkSum = 0;

   // test si c'est la d�connexion qui a eu lieu
   if(!strcmp(ptrTxtIn,trameDisconnect))
   {
    // arr�t imm�diat des moteurs
    commandeMoteur(50);
    commandeServomoteur(50);
    return tramevalide = 0;  
   }
   else
   {
      // test si c'est la connexion qui a lieu
      if(!strcmp(ptrTxtIn,trameConnect))
      {
   	   return tramevalide = 1;
      }
      else
      {
   		// contr�le du d�but de la trame -------------------------------------------
   	   if(ptrTxtIn[0] != START1)  return tramevalide = 0;// 1 er caract�re incorecte
   	   if(ptrTxtIn[1] != START2)  return tramevalide = 0;  // 2 �me caract�re incorecte
   
   	   // contr�le du chescksum ---------------------------------------------------
   	   for(i=2; i<(nbCar-4) ; i++)
   	   {
   		  checkSum = checkSum + ptrTxtIn[i];
   	   }
   	  
   	   if (ptrTxtIn[nbCar-4] != DecToCarHexa (checkSum/16))	// poids fort
   		  return tramevalide = 0;      // checksum incorrect
   	   if (ptrTxtIn[nbCar-3] != DecToCarHexa (checkSum%16)) // poids faible
   		  return tramevalide = 0;      // checksum incorrect
   
   	   
   	   // contr�le de fin de message ----------------------------------------------
   	   if (!((ptrTxtIn[nbCar-2] == STOP1) || (ptrTxtIn[nbCar-2] == STOP2))) 
   		  return tramevalide = 0; 
   
   	   // retourne l'information de validit� --------------------------------------
   	   return tramevalide;
      } 
   }        
      
} // controleTrame ------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   Traitement ()
  -----------------------------------------------------------------------------
   Descriptif: Fonction permettant de faire le traitement de la trame re�u

   Entr�e    : noCMD, valeur de la commande
               ptrIn, l'addresse de la cha�ne re�ue
               ptrOut, l'adresse de la chaine r�ponse
               val, la valeur que l'on veut envoyer (pour les requ�tes)
   Sortie    : --
-*---------------------------------------------------------------------------*/
void traitement (unsigned char noCMD,char* ptrIn, char* ptrOut,unsigned char val) 
{
   unsigned char valeur;
   unsigned char checkSum = 0;
   unsigned char i; 
   unsigned char j = 6;
   unsigned char calcule_chk = 0;


   switch(noCMD)
   {
      case CMD_MO : // la commande avancer

           valeur = CarHexaToDec(ptrIn[4]) * 100;
           valeur = valeur + CarHexaToDec(ptrIn[5]) * 10; 
           valeur = valeur + CarHexaToDec(ptrIn[6]); 
           commandeMoteur(valeur);

           // consitue la trame de r�ponse
           strcpy(ptrOut,"*$cmOKxx\r\n");
		    
           calcule_chk = 1;

      break;           

      case CMD_TO : // la commande pour tourner
         
         valeur = CarHexaToDec(ptrIn[4]) * 100;
         valeur = valeur + CarHexaToDec(ptrIn[5]) * 10; 
         valeur = valeur + CarHexaToDec(ptrIn[6]); 
         commandeServomoteur(valeur);

         // constitue la trame de r�ponse
         strcpy(ptrOut,"*$ctOKxx\r\n"); 

		   calcule_chk = 1;

      break;

      case CMD_AP : // la commande pour activer l'antipatinage

         antipatinage = 1; // activation de l'antipatinage  
		   calcule_chk = 0;

      break;

      case REQ_VI : // requ�te de vitesse de la voiture

         // consitue la trame de r�ponse
         strcpy(ptrOut,"*$viyyxx\r\n");
         ptrOut[4] = (val/10) + 0x30;
         ptrOut[5] = (val%10) + 0x30;

         calcule_chk = 1;

      break;
	  
	   case CMD_CONNEXION : // si on re�oit une commande de connexion
						// on pr�vient le smartphone que nous somme bien connect�
		 strcpy(ptrOut,"Connect�");
		 calcule_chk = 0;
	  break;
   }

   if(calcule_chk)
   {     
		// calcul du checksum
	   for(i=2 ; i<j ; i++)
	   {
		  checkSum = checkSum + ptrOut[i];
	   }

	   ptrOut [6] = DecToCarHexa (checkSum/16);   // poids fort
	   ptrOut [7] = DecToCarHexa (checkSum%16);   // poids faible
   }   
      
} // Traitement ---------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   DecToCarHexa ()
  -----------------------------------------------------------------------------
   Descriptif: transforme une valeur d�cimale (0..15) par le caract�res 
               hexad�cimal ('0'..'F')
   Entr�e    : val, la valeur d�cimale                                 (0..15)
   Sortie    : le caract�re hexad�cimal                             ('0'..'F')
-*---------------------------------------------------------------------------*/
unsigned char DecToCarHexa (unsigned char val) 
{

   if (val > 9)           // si compris en 'A' et 'F' (10 et 15)
      return (val + 'A' - 10);
   else return (val + '0');       // sinon entre '0' et '9'
      
} // DecToCarHexa -------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   CarHexaToDec ()
  -----------------------------------------------------------------------------
   Descriptif: transforme un caract�re hexad�cimal ('0'..'F') par une valeurs 
               d�cimales (0..15).
   Entr�e    : carHexa, le caract�res hexad�cimal                    ('0'..'F')
   Sortie    : la valeur d�cimale calcul�e                              (0..15)
-*---------------------------------------------------------------------------*/
unsigned char CarHexaToDec (unsigned char carHexa) 
{

   if ((carHexa) >='A')
      return (carHexa - 'A' + 10);
   else return (carHexa - '0');

} // CarHexaToDec --------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   EmissionUART ()
  -----------------------------------------------------------------------------
   Descriptif: Fonction permettant l'envoi du premier caract�re et ainsi, faire
               une proc�dure d'envoi de la trame

   Entr�e    : ptrText, l'adresse de la cha�ne � envoyer
   Sortie    : --
-*---------------------------------------------------------------------------*/
void EmissionUART (char* ptrText) 
{
   SBUF0 = ptrText[0];  // envoi 1er caract�re
} // EmissionUART -------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   InterruptionUART ()
  -----------------------------------------------------------------------------
   Descriptif: Interruption qui se d�clence lorsqu'on envoi ou re�oi une trame
               Si r�ception : enregistrement 

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void InterruptionUART () interrupt INTERRUPT_UART0
{
   static unsigned char posSend = 1;   // position dans le buffer d'�mission
   

   if(RI0)  // si r�ception de caract�re
   {
      
      TMR3CN &= ~0x04;     // stop le Timeout
      TMR3RLH = LOADVALUE3 / 256;   // charge la valeure de pr�chargement
      TMR3RLL = LOADVALUE3 % 256;  
      TMR3CN |= 0x04;      // start Timeout

      bufferIn[posRec++] = SBUF0;   // m�morise les donn�es
      
      if(posRec >= BUFFER_MAX)      // test de d�bordement
      {
         posRec = 0;   
      }   

      RI0 = 0;    // reset flag d'interruption en r�ception                 
   }

   if(TI0)  // si transmission de caract�re
   {
      if(bufferOut [posSend])    // si on a pas atteint la fin de la chaine AZT   
      {
         SBUF0 = bufferOut[posSend++];    // envoi la donn�e suivante         
      }   
      else
      {
         posSend = 1;      // position initale
         sendBusy = 0;     // signal plus d'�mission en cours
      }

      TI0 = 0;    // reset flag d'interruption en �mission
   }
      
} // InterruptionUART ---------------------------------------------------------

/*---------------------------------------------------------------------------*-
   ClockInit ()
  -----------------------------------------------------------------------------
   Descriptif: Initialisation du mode de fonctionnement du clock syst�me 
                  SYSCLK : Oscillateur interne � 12 MHz
   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void ClockInit ()
{
                     // +--------- non utilis�
                     // |+++------ S�lection du clock USB 
                     // ||||           (000 : clock multiplier    = 48 MHz)
                     // ||||++---- non utilis�s 
                     // ||||||++-- choix du clock syst�me
                     // ||||||||       (00 : Oscillateur interne  = 12 MHz)
                     // ||||||||       (01 : Oscillateur externe  = x  MHz)
                     // ||||||||       (10 : clock multiplier / 2 = 24 MHz)
                     // ||||||||       (11 : r�serv�                      )
   CLKSEL = 0x00;    // 00000000  
    
                     // +--------- clock interne autoris� 
                     // |+-------- en lecture seule 1 : signal que oscillateur 
                     // ||         interne fonctionne � sa valeur de prog.
                     // ||+------- 1 : suspend l'oscillateur interne
                     // |||+++---- non utilis�s
                     // ||||||++-- choix du diviseur :
                     // ||||||||       (00 : Osc/8 -> f =  1.5 MHz)
                     // ||||||||       (01 : Osc/4 -> f =  3   MHz)
                     // ||||||||       (10 : Osc/2 -> f =  6   MHz)
                     // ||||||||       (11 : Osc/1 -> f = 12   MHz)
   OSCICN = 0x83;    // 10000011 
   
} // ClockInit ----------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   PortInit ()
  -----------------------------------------------------------------------------
   Descriptif: Initialisation du mode de fonctionnement des ports 

      P0.0           : --
      P0.1           : --
      P0.2           : entr�e  (OC) : RTS 
      P0.3           : sortie  (PP) : CTS 
      P0.4           : sortie  (PP) : UART_TX 
      P0.5           : entr�e  (OC) : UART_RX 
      P0.6           : entr�e  (OC) : interruption externe INT0
      P0.7           : --

      P1.0           : entr�e (AN)  : acc�l�rom�tre axe X
      P1.1           : entr�e (AN)  : acc�l�rom�tre axe Y
      P1.2           : entr�e (AN)  : acc�l�rom�tre axe Z
      P1.3           : sortie (PP)  : SPI_CLK
      P1.4           : entr�e (OC)  : SPI_MISO
      P1.5           : sortie (PP)  : SPI_MOSI
      P1.6           : sortie (PP)  : SPI_NSS1 ->RTC
      P1.7           : sortie (PP)  : SPI_NSS2 ->carte SD
      
      P2.0           : sortie  (PP) : commande servomoteur
      P2.1           : sortie  (PP) : commande moteur
      P2.2           : --
      P2.3           : --
      P2.4           : entr�e  (AN) : mesure tension accu
      P2.5           : entr�e  (AN) : mesure tension driver
      P2.6           : entr�e  (AN) : mesure de la distance
      P2.7           : sortie  (PP) : PIN DE DEBUG
      
      avec : (OC) = Open Collector    (PP) = Push Pull    (AN) = Analogique

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void PortInit () 
{
   // PnMDIN : S�lection des entr�es ------------------------------------------
   // 	0 : entr�es analogiques 
   // 	1 : entr�es num�riques (open collector)
   P1MDIN = 0xF8;       // entr�e sur P1.0, P1.1 et P1.2
   P2MDIN = 0x8F;       // entr�e sur P2.4. P2.5 et P2.6

   // PnMDOUT : S�lection des sorties num�riques ------------------------------
   P0MDOUT = 0x04;      // sortie P0.2 en push-pull
   P1MDOUT = 0x80;      // sortie P1.7 en push-pull
   P2MDOUT = 0x83;      // sortie P2.0, P2.1 et P2.7 en sortie push-pull

   // PnSKIP : R�servation des entr�es/sorties utilis�es ----------------------
   P0SKIP = 0xCF;       // r�servation de P0.2 et P0.3
   P1SKIP = 0x87;       // r�servation de P1.0, P1.1, P1.2 et P1.7 

   // XBRn : Autorisation des ressources internes -----------------------------
   XBR0 |= 0x03;        // autorisation de l'UART (Tx = P0.4 & Rx = P0.5)
                        // et du SPI (CLK = P1.3, MISO = P1.4, MOSI = P1.5
                        // et NSS1 = P1.6
   XBR1 |= 0x42;        // autorise le fonctionnement du crossbar, PCA0 et PCA1

} // PortInit ----------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   Timeout ()
  -----------------------------------------------------------------------------
   Descriptif: Fonction d'interruption du timer 3, gestion de la fin de trame
               sur timeout apr�s 3.5 millisecondes d'absence de caract�res

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void Timeout () interrupt INTERRUPT_TIMER3
{    
   TMR3CN &= ~0x04;     // stop le Timeout
   TMR3CN &= 0x7F;      // clear flag d'interruption poids fort et faible 

   bufferIn[posRec] = 0;   // transforme en chaine de caract�res
   nbCaracteres = posRec;  // m�morise le nombre de caract�res re�ues
   strRec = 1;             // signal une trame re�ue 
   posRec = 0;             // RAZ position de m�morisation   

} // Timeout ------------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   Basetemps ()
  -----------------------------------------------------------------------------
   Descriptif: Fonction d'interruption permettant la mise en place d'une base
               de temps de 1 milliseconde

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void Basetemps () interrupt INTERRUPT_TIMER2
{        
   TF2H = 0;   // efface le flag d'interruption

   cptTempo500ms++;

   // si on a atteint 500ms
   if(cptTempo500ms > 499)
   {
      cptTempo500ms = 0;
      flagLectureVitesse = 1; // demande de lecture de la vitesse
      AD0BUSY = 1; // start la conversion AD
   }
   
} // Basetemps ----------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   CompteVitesse ()
  -----------------------------------------------------------------------------
   Descriptif: Fonction d'interruption permettant de compter le nombre de tour
               que l'arbre de rotation effectue

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void CompteVitesse () interrupt INTERRUPT_INT0
{  
   cptVitesse++;       
} // CompteVitesse ------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   Timer3Init ()
  -----------------------------------------------------------------------------
   Descriptif: Initialisation du mode de fonctionnement du timer 3 pour mettre
               en place un Timeout qui se d�clence lorsqu'une trame compl�te 
               de l'UART est re�ue

               - Timer 3 16 bits auto-reload avec un start soft
               - Clock = 12Mhz
               - Temps d�sir� = 3.5 ms (environ)

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void Timer3Init ()
{

   TMR3CN &= 0x00;   // nettoyage complet du registre du timer 3

                     // +--------- TF3H: Mis � 1 lorsqu'une interruption
                     // |                intervient sur bits de poids fort
                     // |
                     // |+-------- TF3L: Mis � 1 lorsqu'une interruption
                     // ||               intervient sur bits de poids faible
                     // ||
                     // ||+------- d�sactive interruptions 8 bits poids faibles
                     // |||
                     // |||+------ pas utilis� : pour USB        
                     // ||||        
                     // |||| +---- Timer 3 fonctionne en 16 bits autoreload         
                     // |||| |
                     // |||| |+--- Timer 3 pas encore autoris� 
                     // |||| ||
                     // |||| ||  
   TMR3CN |= 0x00;   // 0000 00xx 

   CKCON |= 0x40;  

   TMR3H = LOADVALUE3 / 256;   // valeure initiale
   TMR3L = LOADVALUE3 % 256;
   
   TMR3RLH = LOADVALUE3 / 256;   // charge la valeure de pr�chargement
   TMR3RLL = LOADVALUE3 % 256;         
      
} // Timer3Init ---------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   Timer1Init ()
  -----------------------------------------------------------------------------
   Descriptif: Initialisation du mode de fonctionnement du timer 1 pour l'UART

                  - Timer1 8 bits auto-reload avec un start soft
                  - Baudrate = 115200 bauds --> clock timer 1 = 230400 Hz
                                          --> P�riode d�sir�e = 4.34 us
                  - clock = 12 MHz 
   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void Timer1Init ()
{
   //--- Bloque le fonctionnement du timer 1 ----------------------------------
   TR1 = 0;   // Stop le timer 1
   ET1 = 0;   // Inhibe l'interruption du timer 1 
 
   //--- Configuration du mode de fonctionnement ------------------------------
   TMOD &= 0x00;     // Reset l'initialisation du timer 1

  	                  // +--------- Choix du mode de d�clenchement :
                     // |                0 : Start soft
                     // |                1 : Start externe 
                     // |+-------- Choix Compteur/Timer :
                     // ||               0 : fonction timer 
                     // ||               1 : fonction compteur  
                     // ||++------ choix du mode de fonctionnement
                     // ||||           (00 : mode 0 = 13 bits           )
                     // ||||           (01 : mode 1 = 16 bits           )
                     // ||||         ->(10 : mode 2 = 8 bits auto-reload)
                     // ||||           (11 : mode 3 = pas utilis�       )
                     // ||||
   TMOD |= 0x21;     // 0010xxxx  
 
   //--- Configuration du clock du timer 1 ------------------------------------
   CKCON &= ~0x0B;   // reset config clock timer 1 
                     // !! Reset le pr�-diviseur utilisable par le timer 0

                     // ++-------- non utilis�s : uniquement pour le timer 3
                     // ||++------ non utilis�s : uniquement pour le timer 2
                     // ||||+----- Pr�diviseur d�sactiv�
                     // |||||+---- non utilis�s : uniquement pour le timer 0 
                     // ||||||++-- choix de la pr� division 
                     // ||||||||       (00 : SYSCLK/12       )
                     // ||||||||       (01 : SYSCLK/4        )
                     // ||||||||       (10 : SYSCLK/48       )
                     // ||||||||       (11 : External Clock/8)
                     // |||||||| 
   CKCON |= 0x08;    // xxxx1xxx    

   TH1 = TL1 = LOADVALUE;   // Charge la valeur initiale 
         
   //--- "Nettoyage" avant utilisation ----------------------------------------
   TF1 = 0;    // Efface une interruption r�siduelle


} // Timer1Init ---------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   Timer2Init ()
  -----------------------------------------------------------------------------
   Descriptif: Initialisation du mode de fonctionnement du timer 2 pour avoir
			   une base de temps de 1 milliseconde

                  - Timer2 16 bits auto-reload avec un start soft
                  - base de temps toutes les 1 ms
                  - clock = 12 MHz 
   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void Timer2Init ()
{
	
	TR2 = 0;	// stop le timer 2
    ET2 = 0;	// inhibe le timer 2	

	TF2LEN = 0; // interruption bit poids faible non autoris�
	T2SOF = 0;  // n'autorise pas l'USB	
	T2SPLIT = 0;	// utilisation du mode 16 bits
	
	CKCON |= 0x30;	// utilisation du clock syst�me non divis�

  	
	TMR2H = LOADVALUE2 / 256;		// valeur initiale poids fort
    TMR2L = LOADVALUE2 % 256;  	// valeur initiale poids faible
	
			
	TMR2RLH = LOADVALUE2 / 256;	// valeur de pr�chargement poids fort
    TMR2RLL = LOADVALUE2 % 256;	// valeur de pr�chargement poids faible
   
    TF2H = 0; 	// efface une interruption r�siduelle

} // Timer2Init ---------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   IRQInit ()
  -----------------------------------------------------------------------------
   Descriptif: Initialisation du mode de fonctionnement de l'interruption 
               externe 0, sur transition montante et sur P0.6
   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void IRQInit ()
{
   // Interruption externe 0 --------------------------------------------------
   EX0 = 0;                 // Inhibe l'interruption INT0 
   
   IT0 = 1;                 // Interruption actif sur transition. 
   IT01CF = IT01CF & 0xF0;  // Clear bit de configuration INT0  
   
                            //     +----- Etat haut ou transition montante  
                            //     |+++-- INT0 sur bit 6 du port 0 
                            //     |||| 
   IT01CF = IT01CF | 0x0E;  // xxxx1110  0E
   
   IE0 = 0;                 // Efface une interruption r�siduelle 
   IE0 = 0;                 // perte de temps
   IE0 = 0;                 // perte de temps
   
} // IRQInit ----------------------------------------------------------------- 

/*---------------------------------------------------------------------------*-
   UartInit ()
  -----------------------------------------------------------------------------

   Descriptif: Initialisation du mode de fonctionnement de l'UART 
                  - 115'200 bauds
                  - mode 8 bits (pas de parit�)
                  - signal de r�ception seulement si stop bit d�tect�
                  - Timer 1 � 230'400 Hz
                  
                           %%%% -> 115'200 8-N-1 <- %%%%

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void UARTInit () 
{
   Timer1Init ();       // init. Timer 1

   // Configuration de l'UART (!! registre SCON0 !! (donc bit adressable ..))

   S0MODE = 0;       // mode 8 bits de donn�es
   MCE0 = 1;         // signal r�ception seulement si stop bit d�tect� 
   REN0 = 1;         // r�ception autoris�e

   RI0 = 0;          // R.A.Z. flag interruption caract�re re�u    !! Rx !!
   TI0 = 0;          // R.A.Z. flag interruption caract�re envoy�  !! Tx !!
   
} // UartInit -----------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   PCAInit ()
  -----------------------------------------------------------------------------

   Descriptif: Initialisation du mode de fonctionnement des PCA

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void PCAInit () 
{
   // // Initialise CLOCK  ------------------------------------- 
   PCA0MD  &= ~0x8E; //  0xxx/000X     Reset 

                     //   +------------- mode Idle
                     //   |               0 : bloqu�
                     //   |               1 : autoris�
                     //   |    000 ----- SYSCLK/12
                     //   |    001 ----- SYSCLK/4
                     //   |    010 ----- d�bordement du Timer 0
                     //   |    011 ----- flanc montant sur l'entr�e ECI
                     //   |    100 ----- SYSCLK
                     //   |    101 ----- clock externe/8
                     //   |    |||
   PCA0MD |= 0x08;   //   0xxx/100X  

   
   // Initialise PCA0  -------------------------------------
   // choix du mode de fonctionnement du module PCA0  -------------------------
                        //    +----------  PWM160 : d�finition PMW  
                        //    |               0 :  8 bits
                        //    |               1 : 16 bits
                        //    |+---------  ECOM0 : comparateur PCA0
                        //    ||              0 : bloqu�
                        //    ||              1 : autoris�
                        //    ||+--------  mode capture positive
                        //    |||             0 : bloqu�
                        //    |||             1 : autoris�
                        //    |||+--------  mode capture n�gative
                        //    ||||            0 : bloqu�
                        //    ||||            1 : autoris�
                        //    |||| +-----  mode math    
                        //    |||| |          0 : bloqu�
                        //    |||| |          1 : autoris�
                        //    |||| |+----  mode bascule   
                        //    |||| ||         0 : bloqu�
                        //    |||| ||         1 : autoris�
                        //    |||| ||+---  mode PMW       
                        //    |||| |||        0 : bloqu�
                        //    |||| |||        1 : autoris�
                        //    |||| |||+--- interruptions 
                        //    |||| ||||       0 : bloqu�s
                        //    |||| ||||       1 : autoris�s
                        //    |||| ||||
   PCA0CPM0 = 0xC2;     //    1100 0010   // mode PWM 8 bit autoris�

   // condition de d�part : commande PWM � 0 % --> sortie � '1' 
   //                       !!commande avec un niveau bas

   PCA0CPL0  = ~PCA_INIT_SERVO%256;    // valeur de d�part de la consigne PMW = 0 %
   PCA0CPH0  = ~PCA_INIT_SERVO/256;    // consigne PMW = 0 %   



   // Initialise PCA1  ------------------------------------- 
   // choix du mode de fonctionnement du module PCA0  -------------------------
                        //    +----------  PWM160 : d�finition PMW  
                        //    |               0 :  8 bits
                        //    |               1 : 16 bits
                        //    |+---------  ECOM0 : comparateur PCA0
                        //    ||              0 : bloqu�
                        //    ||              1 : autoris�
                        //    ||+--------  mode capture positive
                        //    |||             0 : bloqu�
                        //    |||             1 : autoris�
                        //    |||+--------  mode capture n�gative
                        //    ||||            0 : bloqu�
                        //    ||||            1 : autoris�
                        //    |||| +-----  mode math    
                        //    |||| |          0 : bloqu�
                        //    |||| |          1 : autoris�
                        //    |||| |+----  mode bascule   
                        //    |||| ||         0 : bloqu�
                        //    |||| ||         1 : autoris�
                        //    |||| ||+---  mode PMW       
                        //    |||| |||        0 : bloqu�
                        //    |||| |||        1 : autoris�
                        //    |||| |||+--- interruptions 
                        //    |||| ||||       0 : bloqu�s
                        //    |||| ||||       1 : autoris�s
                        //    |||| ||||
   PCA0CPM1 = 0xC2;     //    1100 0010   // mode PWM 8 bit autoris�

   // condition de d�part : commande PWM � 0 % --> sortie � '1' 
   //                       !!commande avec un niveau bas
   PCA0CPL1  = ~PCA_INIT_MOT%256;    // valeur de d�part de la consigne PMW = 0 %
   PCA0CPH1  = ~PCA_INIT_MOT/256;    // consigne PMW = 0 % 

} // PCAInit -----------------------------------------------------------------

/*---------------------------------------------------------------------------*-
   ADInit ()
  -----------------------------------------------------------------------------

   Descriptif: Initialisation le mode de fonctionnement du convertisseur AD

   Entr�e    : --
   Sortie    : --
-*---------------------------------------------------------------------------*/
void ADInit () 
{	
	// Interruption convertisseur AD -------------------------------------------
   EIE1 &= 0x08;     // Inhibe l'interruption du convertisseur

   // Registre de s�lection des canaux du convertisseur -----------------------
   AMX0P = 0x0E;     // entr�e analogique sur capteur de ligne P2.6
   AMX0N = 0x1F;     // GND sur l'entr�e n�gative (single ended mode)
 
   // Registre de configuration du convertisseur ------------------------------
                     // ++++ +----- (12 MHz / 3MHz) - 1) = 3 (CLKSAR = 3 MHz)
                     // |||| |+---- justification � gauche (lecture 8 bits) 
                     // |||| ||++-- non utilis�s 
   ADC0CF = 0x1C;    // 0001 1100 

   // Registre de contr�le du convertisseur -----------------------------------
                     // +---------- Fonctionnement activ�  
                     // |+--------- 0 : normal track mode 
                     // ||+-------- reset flag interruption de conversion
                     // |||+------- pas d'effect
                     // |||| +----- reset flag interruption du mode fen�tre
                     // |||| |+++-- 000 : start soft avec AD0BUSY = 1
                     // |||| |+++-- 001 : start sur d�bordement du timer 0
                     // |||| |+++-- 010 : start sur d�bordement du timer 2
                     // |||| |+++-- 011 : start sur d�bordement du timer 1
                     // |||| |+++-- 100 : start sur flanc montant entr�e CNVSTR
                     // |||| |+++-- 101 : start sur d�bordement du timer 3
   ADC0CN = 0x80;    // 1000/0000 

   // Registre de contr�le de la tension de r�f�rence -------------------------
                     // ++++ ------ non utilis�s  
                     // |||| +----- choix de la tension de r�f�rence 
                     // |||| |          0 : interne (2.44 V)
                     // |||| |        ->1 : Vcc (3.3 V)
                     // |||| |+---- capteur de temp�rature : d�sactiv�
                     // |||| ||+--- pas n�cessaire, d�ja activ� par ADC0EN
                     // |||| |||+-- r�f�rence interne : activ�e
   REF0CN = 0x08;    // 0000/1000

   AD0INT = 0;       // clear flag d'interruption
   
} // ADInit -------------------------------------------------------------------