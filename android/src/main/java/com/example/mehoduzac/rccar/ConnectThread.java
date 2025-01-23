package com.example.mehoduzac.rccar;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.UUID;

/**
 * Created by MehoDuzac on 13.05.2015.
 * Class permettant la mise en place d'une connexion Bluetooth.
 * Cette class est initialement écrite par Google et modifiée, afin de pouvoir créer
 * une connexion Bluetooth sans utiliser d'UUID
 */
public class ConnectThread extends Thread {
    private final BluetoothSocket mmSocket;
    private final BluetoothDevice mmDevice;

    /**/
    public ConnectThread(BluetoothDevice device){
        BluetoothSocket tmp = null;
        mmDevice = device;

        try {
          /*  Method m = device.getClass().getMethod("createRfcommSocket", new Class[]{int.class});

            tmp = (BluetoothSocket) m.invoke(device, 1);
            */
             // uuid pour une communication série
            tmp = device.createRfcommSocketToServiceRecord(
                    UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"));

        }catch(Exception e){}

        mmSocket = tmp;
    }

    /**
     *
     */
    public void run(){
        try {
            mmSocket.connect();
        }catch (IOException connectException){
            try {
                mmSocket.close();
            }catch (IOException closeException){}
        }
        return;


    }

    public void cancel(){
        try {
            mmSocket.close();
        }catch (IOException e){}
    }

    BluetoothSocket returnSocket(){
        return mmSocket;
    }
}
