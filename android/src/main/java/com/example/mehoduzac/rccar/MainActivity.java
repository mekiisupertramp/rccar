package com.example.mehoduzac.rccar;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Drawable;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.support.v7.internal.view.menu.ListMenuItemView;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.MathContext;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.UUID;


public class MainActivity extends ActionBarActivity {

    // déclaration des objets graphiques
    ImageView imvPokeball = null;
    Button btnDrive = null;
    GestionTrames gestionTrames = new GestionTrames();
    TextView debugServo = null;
    TextView vitesse = null;
    TextView debugMot = null;

    // variables globales
    boolean flag_drive = false;
    float[] acceleromterVector = new float[3];
    float[] magneticVector = new float[3];
    float[] resultMatrix = new float[9];
    float[] valInclinaison = new float[3];
    int[] tabDirection = new int[5];
    int[] tablAcceleration = new int[5];
    int i =0;
    int inclin_direction;
    int inclin_acceleration;
    boolean connexion = false;
    int cpt_ack = 0;
    int tempoVitesse = 1;
    String ack_vitesse = null;
    String ack_servo = null;
    String ack_mot = null;
    int direction = 0;
    int puissance = 0;
    boolean drive = false;

    // éléments qui vont gérer la communication
    int cpt_reponse_fausse;
    boolean enableThreadCommande  = true;
    String acknoledge;
    Thread threadComm;
    Thread timeout;
    Handler handlerComm;
    Handler handlerAff;
    Handler handUI;
    Thread affichage;


    // constantes
    final int OFFSET_POKE_X = 350;
    final int OFFSET_POKE_Y = 340;

    // identificateur de trames
    static final String CM  = "CM";
    static final String CT  = "CT";
    static final String AP  = "AP";
    static final String VI  = "VI";
    static final byte[] REQ_VI = {'*','$','V','I','9','F','\r','\n'};
    static final int ID_CT  = 0;
    static final int ID_CM  = 1;
    static final int ID_VI  = 2;
    static final int ID_AP  = 3;
    static final int ERREUR = 0xFF;

    // éléments qui va afficher mes périphériques Bluetooth
    AlertDialog dialog;
    AlertDialog.Builder builder;
    ArrayList<String> arrayPeripheriques = new ArrayList<String>();
    ArrayAdapter<String> arrayAdapter;
    ListView listView;

    // gestion de la sensibilité
    AlertDialog dialogSensi;
    AlertDialog.Builder builderSensi;
    ArrayList<String> arraySensi = new ArrayList<String>();
    ArrayAdapter<String> arrayAdapter2;
    ListView listViewSensi;
    int sensibility = 40;
    int positionSensi = 1;

    // éléments qui va gérer ma connexion Bluetooth
    BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    BluetoothDevice bluetoothDevice;
    BluetoothSocket bluetoothSocket;
    ConnectThread connectThread;
    ConnectedThread connectedThread;

    // éléments qui vont gérer l'inclinaison
    Sensor magneticField = null;
    Sensor accelerometer = null;
    SensorManager sensorManager;
    SensorEventListener sensorEventListener;



    /**
     * Fonction qui est appelée à la création de l'activité
     * @param savedInstanceState
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // instanciation des objets graphiques
        imvPokeball = (ImageView)findViewById(R.id.imvPokeball);
        imvPokeball.setImageResource(R.drawable.pokeball);
        btnDrive = (Button)findViewById(R.id.btnDrive);
        debugServo = (TextView)findViewById(R.id.mytextView);
        vitesse = (TextView)findViewById(R.id.textView2);
        debugMot = (TextView)findViewById(R.id.textView3);

        // déclaration des différents modes
        arraySensi.add("Hard");
        arraySensi.add("Medium");
        arraySensi.add("Soft");


        // instanciation des capteurs
        sensorManager = (SensorManager)getSystemService(SENSOR_SERVICE);
        magneticField = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
        accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);

        gestionInterruptionCapteur();   // C'est la que tout va se passer pour l'inclinaison


        // lorsqu'on touche le bouton drive---------------------------------------------------------
        btnDrive.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                // test si on vient d'appuyer
                if (event.getAction() == MotionEvent.ACTION_UP) {
					// envoie une trame pour dire que la voiture s'arrête
                    if (connexion){
                        flag_drive = false;
                        // désenregistrement du listener avec nos capteurs
                        sensorManager.unregisterListener(sensorEventListener, accelerometer);
                        sensorManager.unregisterListener(sensorEventListener, magneticField);
                        // la pokeball reprend sa position initiale
                        imvPokeball.setX(OFFSET_POKE_X);
                        imvPokeball.setY(OFFSET_POKE_Y);

                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                try{
                                    connectedThread.write(gestionTrames.trameToSend(50,CM));
                                    Thread.sleep(10);
                                    connectedThread.write(gestionTrames.trameToSend(50,CT));
                                }catch (InterruptedException e){}
                            }
                        }).start();
                    }
                    i=0;
                    direction = 50;
                    puissance = 50;


                } else if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    if (connexion){
                        flag_drive = true;
                        // enregistrement du listener avec nos capteurs
                        sensorManager.registerListener(sensorEventListener, accelerometer,
                                SensorManager.SENSOR_DELAY_UI);
                        sensorManager.registerListener(sensorEventListener, magneticField,
                                SensorManager.SENSOR_DELAY_UI);

                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                             /*   try{
                                    connectedThread.write(gestionTrames.trameToSend(50,CM));
                                    Thread.sleep(10);
                                    connectedThread.write(gestionTrames.trameToSend(50,CT));
                                }catch (InterruptedException e){}*/
                                connectedThread.write(REQ_VI);
                            }
                        }).start();
                        i=0;
                        //direction = 50;
                        //puissance = 50;
                    }
                }
                return true;
            }
        });

        // instanciation de mon thread de timeout---------------------------------------------------
        timeout = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(100);
                   // connectedThread.write(gestionTrames.trameToSend(direction,CT));
                }catch (InterruptedException e){}
            }
        });

        // instanciaction du thread de communication------------------------------------------------
        threadComm = new Thread(new Runnable() {
            @Override
            public void run() {
                while (true){
                    // lecture de la réception
                    Message msg = handlerComm.obtainMessage();
                    msg.obj = connectedThread.read();
                    handlerComm.sendMessage(msg);
                }
            }
        });

        // lorsqu'on reçoit une trame --------------------------------------------------------------
        handlerComm = new Handler(){
            @Override
            public void handleMessage(Message msg){
                super.handleMessage(msg);

                if (flag_drive){
                    // on regarde quelle est la trame de réception
                    switch (gestionTrames.verificationTrame(String.valueOf(msg.obj)))
                    {
                        case ID_CT: // si acknoledge tourner
                            ack_servo = String.valueOf(msg.obj);
                            connectedThread.write(gestionTrames.trameToSend(puissance,CM));
                            break;

                        case ID_CM: // si acknoledge moteur
                            ack_mot = String.valueOf(msg.obj);
                            connectedThread.write(REQ_VI);
                            break;

                        case ID_VI: // si acknoledge vitesse
                            ack_vitesse = String.valueOf(msg.obj);
                            connectedThread.write(gestionTrames.trameToSend(direction,CT));
                         //   Toast.makeText(getApplicationContext(),"ID_VI",Toast.LENGTH_SHORT).show();
                            break;

                        case ERREUR: // si erreur
                         //   Toast.makeText(getApplicationContext(),"Erreur",Toast.LENGTH_SHORT).show();
                            break;
                    }
                    enableThreadCommande = true;
                }
            }
        };

        // thread d'affichage ----------------------------------------------------------------------
        affichage = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    while (true){
                        Thread.sleep(500);
                        //debugMot.setText(ack_mot);
                        //debugServo.setText(ack_servo);
                        //vitesse.setText(ack_vitesse);
                        String[] buffer = new String[3];
                        buffer[0] = ack_mot;
                        buffer[1] = ack_servo;
                        buffer[2] = ack_vitesse;

                        Message msg = handlerAff.obtainMessage();
                        msg.obj = buffer;
                        handlerAff.sendMessage(msg);
                    }
                }catch (Exception e){}
            }
        });
        handlerAff = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                String[] buffer = new String[3];

                buffer = (String[])msg.obj;
                debugMot.setText(buffer[0]);
                debugServo.setText(buffer[1]);

                // envoi de l'ack vitesse à l'UI pour être traité
                Message msg2 = handUI.obtainMessage();
                msg2.obj = buffer[2];
                handUI.sendMessage(msg2);
            }
        };

        // handler pour modification vitesse -------------------------------------------------------
        handUI = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);

                if (String.valueOf(msg.obj).contains("*$"))
                {
                    String[] buffer;
                    buffer = String.valueOf(msg.obj).split("");
                    // conversion en km/h
                    double kmh = Integer.valueOf(buffer[5]) + Integer.valueOf(buffer[6]);
                    kmh = kmh * 3.6;
                    vitesse.setText(String.valueOf(kmh) + "  km/h");
                }
            }
        };
    }

    /**
     * Fonction qui instancie l'interruption des capteurs. A chaque fois que la valeur d'une capteur
     * change, le programme passe dans cette fonction
     */
     void gestionInterruptionCapteur(){

         // lorsque l'un des capteurs change de valeurs
        sensorEventListener = new SensorEventListener() {
            @Override
            public void onSensorChanged(SensorEvent event) {

                // teste si le bouton est maintenu
                if (flag_drive) {

                    // quel capteur a créé l'évenement
                    if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
                        acceleromterVector = event.values; // récupération des valeurs
                        // enregistrement du listener avec l'autre capteur
                        sensorManager.registerListener(sensorEventListener, magneticField,
                                SensorManager.SENSOR_DELAY_UI);
                    } else if (event.sensor.getType() == Sensor.TYPE_MAGNETIC_FIELD) {
                        magneticVector = event.values;
                        sensorManager.registerListener(sensorEventListener, accelerometer,
                                SensorManager.SENSOR_DELAY_UI);
                    }

                    // calcule de l'angle du smartphone
                    // demande la matrice de rotation
                    SensorManager.getRotationMatrix(resultMatrix, null, acceleromterVector,
                            magneticVector);
                    // demande le vecteur d'orientation associé
                    SensorManager.getOrientation(resultMatrix, valInclinaison);

                    // récupération des valeur d'angle
                    inclin_direction = (int) Math.toDegrees(valInclinaison[1]);
                    inclin_acceleration = (int) Math.toDegrees(valInclinaison[2]);

                    // déplacement de la pokeball
                    imvPokeball.setX(OFFSET_POKE_X - (inclin_direction * 6));
                    imvPokeball.setY(OFFSET_POKE_Y - (inclin_acceleration * 6));


                    // si on peut envoyer les valeurs
                    if (enableThreadCommande){
                        tabDirection[i] = gestionTrames.convertValues(inclin_direction,sensibility);
                        tablAcceleration[i] = gestionTrames.convertValues(inclin_acceleration,sensibility);
                        if (i>3){
                            enableThreadCommande = false;
                            int j;
                            direction = 0;
                            puissance = 0;
                            for (j=0;j<5;j++)
                            {
                                direction = direction + tabDirection[j];
                                puissance = puissance + tablAcceleration[j];
                            }
                            direction = direction/5;
                            puissance = puissance/5;
                            gestionCommunication();
                            i = 0;
                        }else {i++;}
                    }

                }
                else{
                    // désenregistrement du listener avec nos capteurs
                    sensorManager.unregisterListener(sensorEventListener,accelerometer);
                    sensorManager.unregisterListener(sensorEventListener,magneticField);
                }
            }

            @Override
            public void onAccuracyChanged(Sensor sensor, int accuracy) {

            }
        };
     }

    void gestionCommunication(){
        try {
            threadComm.start();
            affichage.start();
        }catch (IllegalThreadStateException e){}
    }


    /**
     * Fonction qui est appelée à chaque fois que notre application se met en pause
     */
     @Override
     protected void onPause(){
         super.onPause();
         // désenregistrement du listener avec nos capteurs
         sensorManager.unregisterListener(sensorEventListener, accelerometer);
         sensorManager.unregisterListener(sensorEventListener, magneticField);
     }


    /**
     * Fonction qui sera appelée automatiquement à chaque fois que l'on revient sur l'application
     */
    @Override
    protected void onResume(){
        super.onResume();
        // enregistrement du listener avec nos capteurs
        sensorManager.registerListener(sensorEventListener, accelerometer,
                SensorManager.SENSOR_DELAY_UI);
        sensorManager.registerListener(sensorEventListener, magneticField,
                SensorManager.SENSOR_DELAY_UI);
    }


    /**
     * Fonction qui sera appelée à la création du menu
     * @param menu, le menu sur lequel on a cliqué
     * @return
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }


    /**
     * Fonction qui sera appelé lorsqu'un élément sera sélectionné dans le menu d'options
     * @param item, l'élément sélectionné
     * @return
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //lorsqu'on clique sur connexion dans le menu
        if (id == R.id.connexionBL) {

            // teste si le Bluetooth n'est pas activé ----------------------------------------------
            if (!bluetoothAdapter.isEnabled())
            {
                // dialogue qui nous propose d'activer le Bluetooth
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent,1);
            }else{
                // création du dialogue qui affiche les périphériques
                builder = new AlertDialog.Builder(this);
                builder.setTitle("Périphériques");

                // instanciation de la listview et lien entre la liste de périphérique et la listview
                // en ce servant du arrayadapter
                listView  = new ListView(this);
                arrayAdapter = new ArrayAdapter<String>(this,
                        android.R.layout.simple_selectable_list_item,
                        android.R.id.text1,arrayPeripheriques);

                // création du dialogue
                builder.setView(listView);
                dialog = builder.create();
                dialog.show();

                recherchePeripheriques(); // recherche les périphériques

                // Lorsqu'on clique sur un périphérique du dialogue --------------------------------
                listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                    @Override
                    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {

                        // récupération de l'addresse du périphérique sélectionné
                        String[] perConnect =
                                arrayPeripheriques.get(position).split("\r\n");
                        perConnect = perConnect[1].split("     ");

                        // création de la connexion avec le périphérique souhaité
                        connectThread = new ConnectThread(
                                bluetoothAdapter.getRemoteDevice(perConnect[0]));
                        connectThread.run();

                        // récupération du socket de la connexion
                        bluetoothSocket = connectThread.returnSocket();

                        // arrêt de la recherche de périphériques
                        bluetoothAdapter.cancelDiscovery();
                    }
                });
            }

            return true;
        }



        // lorsqu'on clique sur déconnexion dans le menu -------------------------------------------
        if (id == R.id.deconnexionBL){
            Toast.makeText(getApplicationContext(),"Déconnexion",
                    Toast.LENGTH_SHORT).show();
            connectThread.cancel();
            return true;
        }

        // lorsqu'on clique sur la sensibilité -----------------------------------------------------
        if (id == R.id.sensibilite)
        {
            // création du dialogue qui affiche les périphériques
            builderSensi = new AlertDialog.Builder(this);
            builderSensi.setTitle("Sensibilités");

            // instanciation de la listview et lien entre la liste de périphérique et la listview
            // en ce servant du arrayadapter
            listViewSensi  = new ListView(this);
            arrayAdapter2 = new ArrayAdapter<String>(this,
                    android.R.layout.simple_list_item_checked,
                    android.R.id.text1,arraySensi);

            // création du dialogue
            builderSensi.setView(listViewSensi);
            dialogSensi = builderSensi.create();
            dialogSensi.show();

            // propriétés de la liste
            listViewSensi.setChoiceMode(AbsListView.CHOICE_MODE_SINGLE);

            // mise en place de l'adapteur
            listViewSensi.setAdapter(arrayAdapter2);
            listViewSensi.setItemChecked(positionSensi,true);
            listViewSensi.setActivated(true);

            // lorsqu'on sélectionne un mode -------------------------------------------------------
            listViewSensi.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                @Override
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {

                    switch (position){

                        case 0: // lorsqu'on clique sur Hard
                            sensibility = 20;
                            positionSensi = 0;
                        break;

                        case 1: // lorsqu'on clique sur Medium
                            sensibility = 40;
                            positionSensi = 1;
                        break;

                        case 2: // lorsqu'on clique sur Soft
                            sensibility = 60;
                            positionSensi = 2;
                        break;
                    }
                    // juste pour l'éstetique
                    Thread thread = new Thread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                Thread.sleep(500);
                                dialogSensi.cancel();
                            } catch (InterruptedException e){}
                        }
                    });
                    thread.start();
                }
            });

        }
        return super.onOptionsItemSelected(item);
    }


    /**
     * Fonction permettant de mettre les périphériques connues dans le dialogue et la recherche
     * de nouveaux périphérique Bluetooth
     */
    void recherchePeripheriques(){

        // insertion des périphériques appairés ----------------------------------------------------
        Set<BluetoothDevice> pairedDevice = bluetoothAdapter.getBondedDevices();
        // s'il y a des périphériques déjà appairés
        if (pairedDevice.size() > 0){
            for (BluetoothDevice device : pairedDevice){
                // vérification qu'il n'y ait pas deux fois le même périphériques dans la liste
                if (!arrayPeripheriques.contains(device.getName() +'\r'+'\n'+
                device.getAddress() + "     " + "Appairé")){
                    arrayPeripheriques.add(device.getName() +'\r'+'\n'+
                            device.getAddress() + "     " + "Appairé");
                }
            }
        }

        // start la recherche de périphériques (durée = 12 secondes)
        bluetoothAdapter.startDiscovery();

        // création d'un broadcast pour trouver les périphériques
        final BroadcastReceiver broadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();

                // lorsqu'un périphériques est trouvé ------------------------
                if (BluetoothDevice.ACTION_FOUND.equals(action)){
                    // ajout du périphérique dans la liste
                    bluetoothDevice = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                    // vérification qu'il n'y ait pas deux fois le même périphériques dans la liste
                    if (!(arrayPeripheriques.contains(bluetoothDevice.getName() +'\r'+'\n'+
                            bluetoothDevice.getAddress() + "     " + "Non appairé") ||
                            arrayPeripheriques.contains(bluetoothDevice.getName() +'\r'+'\n'+
                                    bluetoothDevice.getAddress() + "     " + "Appairé"))){
                        arrayPeripheriques.add(bluetoothDevice.getName() +'\r'+'\n'+
                                bluetoothDevice.getAddress() + "     " + "Non appairé");
                    }

                    // insertion des périphériques dans une liste
                    listView.setAdapter(arrayAdapter);
                }

                // lorsqu' on se connect à un périphérique -----------------
                if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(action)) {
                    Toast.makeText(getApplicationContext(),
                            "Connecté", Toast.LENGTH_SHORT).show();
                    dialog.cancel();
                    connexion = true;

                    try {
                        // instanciation de la class permettant de manager la connexion
                        connectedThread = new ConnectedThread(bluetoothSocket);

                    }catch (Throwable e){
                        Toast.makeText(getApplicationContext(),"nop",Toast.LENGTH_LONG).show();
                    }
                }

                // lorsqu' on se déconnecte à un périphérique -----------------
                if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action)){
                    // reset flag de connexion
                    connexion = false;
                    Toast.makeText(getApplication(),
                            "Déconnecté",Toast.LENGTH_SHORT).show();
                }
            }
        };

        // enregistrement du broadcast receiver, afin de savoir si une déconnexion s'est produite
        IntentFilter filter2 = new IntentFilter(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        registerReceiver(broadcastReceiver,filter2);

        // enregistrement du broadcast receiver, afin d'être sûr que la connexion s'est produite
        IntentFilter filter1 = new IntentFilter(BluetoothDevice.ACTION_ACL_CONNECTED);
        registerReceiver(broadcastReceiver,filter1);

        // enregistrement du broadcast receiver pour la recherche de périphériques
        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        registerReceiver(broadcastReceiver,filter);

        // insertion des périphériques dans une liste
        listView.setAdapter(arrayAdapter);
    }


}
