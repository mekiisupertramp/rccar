package com.example.mehoduzac.rccar;

import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.sip.SipAudioCall;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.SizeF;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

import javax.security.auth.callback.Callback;

/**
 * Created by MehoDuzac on 13.05.2015.
 * Class permettant de manager une connexion Bluetooth.
 * Cette class est initialement distribuée par Google.
 */
public class ConnectedThread extends Thread{

    private final BluetoothSocket mmSocket;
    private final InputStream mmInStream;
    private final OutputStream mmOutStream;

    public ConnectedThread(BluetoothSocket socket){
        mmSocket = socket;
        InputStream tmpIn = null;
        OutputStream tmpOut = null;

        // récupération des données
        try {
            tmpIn = socket.getInputStream();
            tmpOut = socket.getOutputStream();
        } catch (IOException e) {}

        mmInStream = tmpIn;
        mmOutStream = tmpOut;
    }




    public String read(){
        char[] buffer = new char[10];
        int bytes;
        boolean flag = true;

        InputStreamReader inputStreamReader = new InputStreamReader(mmInStream);

        while (flag){
            try {
                if (mmInStream.available()>9)
                {
                    bytes = inputStreamReader.read(buffer);
                    flag = false;
                    return String.valueOf(buffer);
                }
            } catch (IOException e) {}

            return null;
        }
        return null;
    }


    public void write(byte[] bytes){
        try {
            mmOutStream.write(bytes);
        } catch (IOException e){}
    }




    public void cancel(){
        try {
            mmSocket.close();
        }catch (IOException e){}
    }

}
