package com.local.cinermt;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.Toast;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

//public class MainActivity extends Activity 
public class MainActivity extends AppCompatActivity
		implements OnClickListener {
	String ip_addr;
	int sport, dport;
	String pin;
	
	private final String IP_ADDR = "127.0.0.1";
	private final int DPORT = 23432;
	private final String MAGIC = "CAR";
	private final String PIN = "cinelerra";
	private final char VER = '\001', ZERO = '\000';
	
    private SharedPreferences prefs;
    
	InetAddress in_adr;
	DatagramSocket socket;
	Handler hndr = new Handler();
	
	
	class sender extends Thread implements Runnable {
		public String data;
		sender(String d) { data = d; }
		
		@Override
   	    public void run() {
			if( socket == null ) return;
			String buf = MAGIC + VER + pin + ZERO + data;
			try {
				DatagramPacket packet =
					new DatagramPacket(buf.getBytes(), buf.length(), in_adr, dport);
				socket.send(packet);
			} catch(Exception e) {
				return;
			}
   		}
	}

	public void send(String data) {
		final Thread net = new sender(data);
		net.start();
	}
	
	boolean has_network() {
	    ConnectivityManager cmgr = (ConnectivityManager)
	            getSystemService(Context.CONNECTIVITY_SERVICE);
	    NetworkInfo netinf =
	            cmgr.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
	    netinf = cmgr.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
	    if( netinf == null ) return false;
	    if(!netinf.isAvailable()) return false;
	    if(!netinf.isConnected()) return false;
	    return true;
	}
	
	boolean create_socket() {
		if(socket != null) socket.close();
		socket = null;
		int retry = 10;
		while( --retry >= 0 ) {
			sport = (int)(Math.random() * (65536-1024)) + 1024;
			try {
				in_adr = InetAddress.getAllByName(ip_addr)[0];
				socket = new DatagramSocket(sport);
				socket.setBroadcast(true);
			} catch(Exception e) { SystemClock.sleep(100);  continue; }
			break;
		}
		return retry >= 0;
	}

	void save_defaults() {
		SharedPreferences.Editor ed = prefs.edit();
		ed.putString("IP_ADDR", ip_addr);
		ed.putString("PIN", pin);
		ed.putInt("PORT", dport);
        ed.commit();
	}
	
	void load_defaults() {
		ip_addr = prefs.getString("IP_ADDR", IP_ADDR);
		pin = prefs.getString("PIN", PIN);
		dport = prefs.getInt("PORT", DPORT);
	}
	
	@Override
	protected void onCreate(Bundle b) {
		super.onCreate(b);
		setContentView(R.layout.activity_main);
		if( !has_network() ) {
			Toast.makeText(this, "Can't access wifi", Toast.LENGTH_LONG).show();
			SystemClock.sleep(5000);
			finish();
		}
        prefs = this.getSharedPreferences("CineRmt", 0);
		load_defaults();
		Intent it = getIntent();
    	String s = it.getStringExtra("IP_ADDR");
    	if( s != null ) ip_addr = s;
    	s = it.getStringExtra("PIN");
    	if( s != null ) pin = s;
    	dport = it.getIntExtra("PORT", dport);
    	if( dport < 1024 ) dport = DPORT;
		if( !create_socket() ) {
			Toast.makeText(this, "Can't access network", Toast.LENGTH_LONG).show();
			SystemClock.sleep(5000);
		}
		ImageButton img = (ImageButton)findViewById(R.id.button0);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button1);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button2);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button3);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button4);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button5);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button6);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button7);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button8);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button9);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.buttonA);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.buttonB);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.buttonC);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.buttonD);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.buttonE);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.buttonF);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.button_dot);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.fast_lt);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.media_up);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.fast_rt);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.menu);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.media_lt);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.pause);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.media_rt);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.slow_lt);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.media_dn);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.slow_rt);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.full_scr);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.stop);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.play);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.rplay);
		img.setOnClickListener(this);

		img = (ImageButton)findViewById(R.id.suspend);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.config);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.exit);
		img.setOnClickListener(this);
		img = (ImageButton)findViewById(R.id.power);
		img.setOnClickListener(this);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.menu_main, menu);
		return true;
	}

	@Override
	public void onClick(View v) {
        if (v instanceof ImageButton) {
            int id = ((ImageButton)v).getId();
			if (id == R.id.stop) { send("stop"); }
			else if (id == R.id.play) { send("play"); }
			else if (id == R.id.rplay) { send("rplay"); }
			else if (id == R.id.button0) { send("key 0"); }
			else if (id == R.id.button1) { send("key 1"); }
			else if (id == R.id.button2) { send("key 2"); }
			else if (id == R.id.button3) { send("key 3"); }
			else if (id == R.id.button4) { send("key 4"); }
			else if (id == R.id.button5) { send("key 5"); }
			else if (id == R.id.button6) { send("key 6"); }
			else if (id == R.id.button7) { send("key 7"); }
			else if (id == R.id.button8) { send("key 8"); }
			else if (id == R.id.button9) { send("key 9"); }
			else if (id == R.id.buttonA) { send("key A"); }
			else if (id == R.id.buttonB) { send("key B"); }
			else if (id == R.id.buttonC) { send("key C"); }
			else if (id == R.id.buttonD) { send("key D"); }
			else if (id == R.id.buttonE) { send("key E"); }
			else if (id == R.id.buttonF) { send("key F"); }
			else if (id == R.id.fast_lt) { send("fast_lt"); }
			else if (id == R.id.media_up) { send("media_up"); }
			else if (id == R.id.fast_rt) { send("fast_rt"); }
			else if (id == R.id.menu) { send("menu"); }
			else if (id == R.id.media_lt) { send("media_lt"); }
			else if (id == R.id.pause) { send("pause"); }
			else if (id == R.id.media_rt) { send("media_rt"); }
			else if (id == R.id.slow_lt) { send("slow_lt"); }
			else if (id == R.id.media_dn) { send("media_dn"); }
			else if (id == R.id.slow_rt) { send("slow_rt"); }
			else if (id == R.id.full_scr) { send("key F"); }
			else {
				save_defaults();
				if (id == R.id.config) {
					Intent it = new Intent(this, ConfigActivity.class);
					it.putExtra("IP_ADDR", ip_addr);
					it.putExtra("PIN", pin);
					it.putExtra("PORT", dport);
					startActivity(it);
				}
				else if (id == R.id.suspend)
					send("suspend");
				else if (id == R.id.power)
					send("power");
				else if (id != R.id.exit)
					return;
				finish();
			}
		}
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		super.onOptionsItemSelected(item);
		int id = item.getItemId();
		if (id == R.id.menu_exit) {
			save_defaults();
			finish();
			return true;
		}
		return false;
	}
	
	@Override
	public void onPause() {
		super.onPause();
		save_defaults();
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		save_defaults();
	}
}
