package com.example.lutao.cmakejni.echo;


import com.example.lutao.cmakejni.R;

/**
 * Echo server.
 * @author Onur Cinar
 */
public class EchoServerActivity extends AbstractEchoActivity {
	/**
	 * Constructor.
	 */
	public EchoServerActivity() {
		super(R.layout.activity_echo_server);
	}

	protected void onStartButtonClicked() {
		Integer port = getPort();
		if (port != null) {
			ServerTask serverTask = new ServerTask(port);
			serverTask.start();
		}
	}

	/**
	 * Starts the TCP server on the given port.
	 * @param port
	 * @throws Exception
	 */
	private native void nativeStartTcpServer(int port) throws Exception;

	/**
	 * Starts the UDP server on the given port.
	 * @param port
	 * @throws Exception
	 */
	private native void nativeStartUdpServer(int port) throws Exception;

	/**
	 * Server task.
	 */
	private class ServerTask extends AbstractEchoTask {
		/** Port number. */
		private final int port;
		
		/**
		 * Constructor.
		 * @param port
		 */
		public ServerTask(int port) {
			this.port = port;
		}

		protected void onBackground() {
			logMessage("Starting server.");

			try {
				 nativeStartTcpServer(port);
//				nativeStartUdpServer(port);
			} catch (Exception e) {
				logMessage(e.getMessage());
			}

			logMessage("Server terminated.");
		}
	}
}
