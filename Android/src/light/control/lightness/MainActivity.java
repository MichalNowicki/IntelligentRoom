package light.control.lightness;

import java.net.InetAddress;
import java.net.UnknownHostException;









import light.control.lightness.ip_port;
import light.control.lightness.R;
import light.control.lightness.TCPClient;
import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnKeyListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.TextView;
import android.view.KeyEvent;

public class MainActivity extends Activity {
	
	 private TCPClient mTcpClient;
	 private boolean connected = false;
	 long last = System.currentTimeMillis();
	 
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		buttonInit();
		progressBarInit();
		resetImage();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	void buttonInit()
	{
		// Connect
		final Button buttonConnection = (Button) findViewById(R.id.buttonConnect);
		final EditText EditTextIP = (EditText) findViewById(R.id.editTextIP);
		final EditText EditTextPort = (EditText) findViewById(R.id.editTextPort);
		
		final String name = EditTextIP.getText().toString();
		final int port = Integer.parseInt( EditTextPort.getText().toString() );
		
        buttonConnection.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	// If not connected
            	if ( !connected )
            	{
	            	InetAddress adres = null;
	            	try {
						adres = InetAddress.getByName(name);
						new connectionTCP().execute(new ip_port(adres, port));
					} catch (UnknownHostException e) {
						e.printStackTrace();
					}
            	}
            	else
            	{
            		mTcpClient.sendMessage("X");
            		connected = false;
            		mTcpClient.setRun(false);
            		resetProgressBar();
            		resetImage();
            		buttonConnection.setText("Connect");
            	}
            }
        });
        
        final EditText editTextCLInput = (EditText) findViewById(R.id.editTextCommandLineInput);
        
        class DoneOnEditorActionListener implements OnEditorActionListener {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                	if (connected)
                    {
                		
	                	String command = editTextCLInput.getText().toString();
	                	mTcpClient.sendMessage(command);
	               	    editTextCLInput.setText("");
	               	  
	               	    final TextView textViewConsole = (TextView) findViewById(R.id.textViewConsole);
	               	    textViewConsole.setText( textViewConsole.getText() + command + "\n" );
	              	  	InputMethodManager imm = (InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);
	              	  	imm.hideSoftInputFromWindow(textViewConsole.getWindowToken(), 0);
	                    return true;    
                    }
                }
                return false;
            }
        }
        
        editTextCLInput.setOnEditorActionListener(new DoneOnEditorActionListener());
       
        
        
        
	}
	
	
	void progressBarInit()
	{
		SeekBar lightControlBar = null;
		lightControlBar = (SeekBar) findViewById(R.id.settingBar);
		lightControlBar.setMax(99);
		 
		((SeekBar)lightControlBar).setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
 
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser){
            	if ( connected )
            	{
            		if ( System.currentTimeMillis() - last > 100)
            		{
            			mTcpClient.sendMessage(String.format("1%02d", progress));
            			imageSet(progress);
            			last = System.currentTimeMillis();
            		}
            	}
            	else 
            		seekBar.setProgress(0);
            }
 
            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				
			}
 
          
        });
		
		
	}
	
	void resetProgressBar()
	{
		SeekBar lightControlBar = null;
		lightControlBar = (SeekBar) findViewById(R.id.settingBar);
		lightControlBar.setProgress(0);
	}
	
	
	void resetImage()
	{
		imageSet(0);
	}
	
	void imageSet(int value)
	{
		final TextView view = (TextView) findViewById(R.id.textViewPercent);
		view.setText(Integer.toString(value)+"%");
		
		final ImageView imgView = (ImageView) findViewById(R.id.imageView1);
		String img[] =  {"lvl0", "lvl1", "lvl2", "lvl3", "lvl4"};

		// changing from 0-99 to 0-4
		value = value/20;
		int id = getResources().getIdentifier(img[value], "drawable", getPackageName());
		imgView.setImageResource(id);

	}
	
	public class connectionTCP extends AsyncTask<ip_port,String,Void> {
   	 
		@Override
		protected Void doInBackground(ip_port... adres) {

			
			mTcpClient = new TCPClient(adres[0],new TCPClient.OnMessageReceived() {
				@Override
				public void messageReceived(String message) {
					publishProgress(message);
				}
			});
			mTcpClient.run();
			
			return null;
		
		}

		@Override
		protected void onProgressUpdate(String... msg) {
		      	
			// Connection is established properly
		    if(msg[0] == "Connected" && connected == false)
			{
				connected = true;
				final Button buttonC = (Button) findViewById(R.id.buttonConnect);
				buttonC.setText("Disconnect");
			}
	  	}
	}
	

}
