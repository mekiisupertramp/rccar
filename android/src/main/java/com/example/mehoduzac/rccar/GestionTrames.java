package com.example.mehoduzac.rccar;

/**
 * Created by MehoDuzac on 14.05.2015.
 */
public class GestionTrames {

    // les caractères de contrôle de trames
    private static final char START1 = '*';
    private static final char START2 = '$';
    private static final char STOP1 = '\r';
    private static final char STOP2 = '\n';

    // identificateur de trames
    private static final String CM = "CM";
    private static final String CT = "CT";
    private static final String AP = "AP";
    private static final String VI = "VI";

    // messages acknoledge
    private static final String ACK_CM = "*$cmOK6A\r\n";
    private static final String ACK_CT = "*$ctOK71\r\n";
    private static final String ACK_AP = "*$apOK";

    // réglage sensibilité
    private static final int SOFT = 60;
    private static final int MEDIUM = 40;
    private static final int HARD = 20;


    static final int ID_CT  = 0;
    static final int ID_CM  = 1;
    static final int ID_VI  = 2;
    static final int ID_AP  = 3;
    static final int ERREUR = 0xFF;



    /*
    *  Constructeur par défaut : ne fait rien, il est la car sinon l'application plante !
    * */
    public GestionTrames(){}


    /*
    Fonction permettant d'envoyer la trame dans le bon format
     */
    public byte[] trameToSend(int valeur, String idTrame){
        byte[] trame = new byte[11];
        int checkSum=0;

        // début de le trame
        trame[0] = START1;
        trame[1] = START2;

        // mise en place de l'identificateur de trames
        switch (idTrame){
            case CM:
                trame[2] = 'C';
                trame[3] = 'M';
                break;
            case CT:
                trame[2] = 'C';
                trame[3] = 'T';
                break;
            case AP:
                trame[2] = 'P';
                trame[3] = 'M';
                break;
        }

        // mise en place de la valeur
        if (valeur>99)
        {
            trame[4] = '1';
            trame[5] = '0';
            trame[6] = '0';
        }else{
            trame[4] = '0';
            trame[5] = (byte)((valeur/10) + 0x30);
            trame[6] = (byte)((valeur%10) + 0x30);
        }

        // calcule du checksum
        for (int i = 2 ; i<7 ; i++){
            checkSum = checkSum + trame[i];
        }

        // mise en place du checksum
        trame[7] = (byte)(DecToCarHexa((checkSum & 0xFF) / 16));
        trame[8] = (byte)(DecToCarHexa((checkSum & 0xFF) % 16));


        // mise en place caractères de stopes
        trame[9]  = STOP1;
        trame[10] = STOP2;

        return trame;
    }

    /**
     * Méthode qui transforme notre valeur d'angle en pourcentage
     */
    public int convertValues(int valeur, int sensibility){
        int value = valeur;

        switch (sensibility){
            case SOFT :
                // mise en forme de la direction
                if (value<-SOFT) value = -SOFT;
                if (value>SOFT) value = SOFT;
                // conversion de la valeur de direction en pourcentage
                value = value + SOFT;
                value = (int)(value * (float)100/(SOFT*2));
            break;

            case MEDIUM :
                // mise en forme de la direction
                if (value<-MEDIUM) value = -MEDIUM;
                if (value>MEDIUM) value = MEDIUM;
                // conversion de la valeur de direction en pourcentage
                value = value + MEDIUM;
                value = (int)(value * (float)100/(MEDIUM*2));
            break;

            case HARD :
                // mise en forme de la direction
                if (value<-HARD) value = -HARD;
                if (value>HARD) value = HARD;
                // conversion de la valeur de direction en pourcentage
                value = value + HARD;
                value = (int)(value * (float)100/(HARD*2));
            break;
        }

        return value;
    }

    private char DecToCarHexa(int valeur){
        if (valeur>9) return ((char)(valeur + 'A' - 10));
        else return (char)((valeur + '0'));
    }

    /**
     * Vérifie que la trame correspond à l'un des acknoledge
     * @param trame
     * @return
     */
    public int verificationTrame(String trame){

     /*   if (ACK_CT.equals(trame)) return 0;
        else if (ACK_CM.equals(trame)) return 1;
        else if (ACK_AP.equals(trame)) return 2;
        else return 0xFF;*/

        if (trame.equals(ACK_CT)) return ID_CT;
        else if (trame.equals(ACK_CM)) return ID_CM;
        else if (trame.equals(ACK_AP)) return ID_AP;
        else if (trame.contains("vi")) return ID_VI;
        else return 0xFF;

    }

    public String valeurVitesse(String trame){
        char[] buff = trame.toCharArray();
        String vittesse = String.valueOf(buff[4] + buff[5]);

        return vittesse;
    }


}
