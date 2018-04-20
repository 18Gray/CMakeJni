package com.example.lutao.cmakejni.echo;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;

import com.example.lutao.cmakejni.R;

/**
 * Abstract echo activity object.
 */
public abstract class AbstractEchoActivity extends Activity implements
		OnClickListener {
	/** Port number. */
	protected EditText portEdit;

	/** Server button. */
	protected Button startButton;

	/** Log scroll. */
	protected ScrollView logScroll;

	/** Log view. */
	protected TextView logView;

	/** Layout ID. */
	private final int layoutID;

	/**
	 * Constructor.
	 * @param layoutID
	 */
	public AbstractEchoActivity(int layoutID) {
		this.layoutID = layoutID;
	}

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(layoutID);

		portEdit = (EditText) findViewById(R.id.port_edit);
		startButton = (Button) findViewById(R.id.start_button);
		logScroll = (ScrollView) findViewById(R.id.log_scroll);
		logView = (TextView) findViewById(R.id.log_view);

		startButton.setOnClickListener(this);
	}

	public void onClick(View view) {
		if (view == startButton) {
			onStartButtonClicked();
		}
	}

	/**
	 * On start button clicked.
	 */
	protected abstract void onStartButtonClicked();

	/**
	 * Gets the port number as an integer.
	 * @return port number or null.
	 */
	protected Integer getPort() {
		Integer port;

		try {
			port = Integer.valueOf(portEdit.getText().toString());
		} catch (NumberFormatException e) {
			port = null;
		}

		return port;
	}

	/**
	 * Logs the given message.
	 * @param message
	 */
	protected void logMessage(final String message) {
		runOnUiThread(new Runnable() {
			public void run() {
				logMessageDirect(message);
			}
		});
	}

	/**
	 * Logs given message directly.
	 * @param message
	 */
	protected void logMessageDirect(final String message) {
		logView.append(message);
		logView.append("\n");
		logScroll.fullScroll(View.FOCUS_DOWN);
	}
	
	/**
	 * Abstract async echo task.
	 */
	protected abstract class AbstractEchoTask extends Thread {
		/** Handler object. */
		private final Handler handler;
		
		/**
		 * Constructor.
		 */
		public AbstractEchoTask() {
			handler = new Handler();
		}
		
		/**
		 * On pre execute callback in calling thread.
		 */
		protected void onPreExecute() {
			startButton.setEnabled(false);
			logView.setText("");
		}

		public synchronized void start() {
			onPreExecute();			
			super.start();
		}

		public void run() {
			onBackground();
			handler.post(new Runnable() {
				public void run() {
					onPostExecute();
				}
			});
		}

		/**
		 * On background callback in new thread.
		 */
		protected abstract void onBackground();

		/**
		 * On post execute callback in calling thread.
		 */
		protected void onPostExecute() {
			startButton.setEnabled(true);
		}
	}

	static {
		System.loadLibrary("Echo");
	}
}
