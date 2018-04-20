package com.example.lutao.cmakejni.echo;

import android.os.Bundle;
import android.widget.EditText;

import com.example.lutao.cmakejni.R;

/**
 * Echo client.
 * @author Onur Cinar
 */
public class EchoClientActivity extends AbstractEchoActivity {	
	/** IP address. */
	private EditText ipEdit;

	/** Message edit. */
	private EditText messageEdit;

	/**
	 * Constructor.
	 */
	public EchoClientActivity() {
		super(R.layout.activity_echo_client);
	}

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		ipEdit = (EditText) findViewById(R.id.ip_edit);
		messageEdit = (EditText) findViewById(R.id.message_edit);
	}

	protected void onStartButtonClicked() {
		String ip = ipEdit.getText().toString();
		Integer port = getPort();
		String message = messageEdit.getText().toString();

		if ((0 != ip.length()) && (port != null) && (0 != message.length())) {
			ClientTask clientTask = new ClientTask(ip, port, message);
			clientTask.start();
		}
	}

	/**
	 * Starts the TCP client with the given server IP address and port number,
	 * and sends the given message.
	 * @param ip
	 * @param port
	 * @param message
	 * @throws Exception
	 */
	private native void nativeStartTcpClient(String ip, int port, String message)
			throws Exception;

	/**
	 * Starts the UDP client with the given server IP address and port number.
	 * @param ip
	 * @param port
	 * @param message
	 * @throws Exception
	 */
	private native void nativeStartUdpClient(String ip, int port, String message)
			throws Exception;

	/**
	 * Client task.
	 */
	private class ClientTask extends AbstractEchoTask {
		/** IP address to connect. */
		private final String ip;

		/** Port number. */
		private final int port;

		/** Message text to send. */
		private final String message;

		/**
		 * Constructor.
		 * @param ip
		 * @param port
		 * @param message
		 */
		public ClientTask(String ip, int port, String message) {
			this.ip = ip;
			this.port = port;
			this.message = message;
		}

		protected void onBackground() {
			logMessage("Starting client.");

			try {
				 nativeStartTcpClient(ip, port, message);
//				nativeStartUdpClient(ip, port, message);
			} catch (Throwable e) {
				logMessage(e.getMessage());
			}

			logMessage("Client terminated.");
		}
	}
}
