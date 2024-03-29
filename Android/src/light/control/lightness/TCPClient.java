package light.control.lightness;

import android.util.Log;
import java.io.*;
import java.net.InetAddress;
import java.net.Socket;
 
 
public class TCPClient {
 
    private String serverMessage;
    public static String SERVERIP;
    public static int SERVERPORT;
    private OnMessageReceived mMessageListener = null;
    private boolean mRun = false;
 
    PrintWriter out;
    BufferedReader in;
 
    /**
     *  Constructor of the class. OnMessagedReceived listens for the messages received from server
     *  Setting up the server's IP and PORT
     */
    public TCPClient(ip_port server, OnMessageReceived listener) {
        mMessageListener = listener;
        SERVERIP = server.getIP().toString().substring(1);
        SERVERPORT = server.getPORT();
    }
 
    /**
     * Sends the message entered by client to the server
     */
    public void sendMessage(String message){
        if (out != null && !out.checkError()) {
            out.println(message);
            out.flush();
            Log.e("TCP Client", "C: Sent.");
        }
    }
    
    /**
     * Ending receiving, closing socket and ending ASync Thread
     */
    public void stopClient(){
        mRun = false;
    }
    
    public void setRun(boolean value)
    {
    	mRun = value;
    }
 
    /**
     * Running TCP connection
     */
    public void run() {
 
        mRun = true;
 
        try {
            //here you must put your computer's IP address.
            InetAddress serverAddr = InetAddress.getByName(SERVERIP);
 
            Log.d("TCP Client", "C: Connecting...");
 
            //create a socket to make the connection with the server
            Socket socket = new Socket(serverAddr, SERVERPORT);
 
            mMessageListener.messageReceived("Connected");
            
            try {
 
                //send the message to the server
                out = new PrintWriter(new BufferedWriter(new OutputStreamWriter(socket.getOutputStream())), true);
 
                //receive the message which the server sends back
                in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                
                Log.e("TCP Client", "C: Listening");
                
                //in this while the client listens for the messages sent by the server
               
                serverMessage = in.readLine();
                 
                if (serverMessage != null && mMessageListener != null) {
                	//call the method messageReceived from MyActivity class
                    mMessageListener.messageReceived(serverMessage);
                }
                    
                while (mRun) 
                {
                	 serverMessage = in.readLine();
                     
                     if (serverMessage != null && mMessageListener != null) {
                     	Log.e("TCP Client", "Recieived: " + serverMessage);
                     }
                	
                };
                
 
            } catch (Exception e) {
 
                Log.e("TCP", "S: Error", e);
 
            } finally {
                //the socket must be closed. It is not possible to reconnect to this socket
                // after it is closed, which means a new socket instance has to be created.
                socket.close();
            }
 
        } catch (Exception e) {
 
            Log.e("TCP", "C: Error", e);
 
        }
 
    }
 
    /**
     * ASync must implement this interface in order to process data
     */
    public interface OnMessageReceived {
        public void messageReceived(String message);
    }
}

