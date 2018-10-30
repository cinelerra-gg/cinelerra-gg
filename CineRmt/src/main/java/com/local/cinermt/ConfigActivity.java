package com.local.cinermt;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;

public class ConfigActivity extends Activity implements OnClickListener {
	String ip_addr;
	String pin;
	int port;
	EditText etxt_ip_addr;
	EditText etxt_pin;
	EditText etxt_port;

	@Override
	protected void onCreate(Bundle b) {
		super.onCreate(b);
		
		Intent it = getIntent();
		ip_addr = it.getStringExtra("IP_ADDR");
		pin = it.getStringExtra("PIN");
		port = it.getIntExtra("PORT",0);

		setContentView(R.layout.activity_config);
		etxt_ip_addr = (EditText)findViewById(R.id.ip_addr);
		etxt_pin = (EditText)findViewById(R.id.pin);
		etxt_port = (EditText)findViewById(R.id.port);

		etxt_ip_addr.setText(ip_addr);
		etxt_pin.setText(pin);
		etxt_port.setText(String.format("%d", port));
		
		Button btn = (Button)findViewById(R.id.back);
		btn.setOnClickListener(this);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.menu_main, menu);
		return true;
	}

	@Override
	public void onClick(View v) {	
        if (v instanceof Button) {
            int id = ((Button)v).getId();
			if (id == R.id.back) {
				ip_addr = etxt_ip_addr.getText().toString();
				pin = etxt_pin.getText().toString();
				try {
					port = Integer.valueOf(etxt_port.getText().toString());
				} catch(Exception e) { port = 0; }
				
				Intent it = new Intent(this,MainActivity.class);
				it.putExtra("IP_ADDR", ip_addr);
				it.putExtra("PIN", pin);
				it.putExtra("PORT", port);
				startActivity(it);
				finish();
			}
        }
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		super.onOptionsItemSelected(item);
		int id = item.getItemId();
		if (id == R.id.menu_exit) {
			finish();
			return true;
		}
		return false;
	}
}
