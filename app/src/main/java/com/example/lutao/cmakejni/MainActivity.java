package com.example.lutao.cmakejni;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity
{
    EditText edit_thread_count, edit_iter_count;
    Button btn_start_thread;
    TextView tv_log;

    // Used to load the 'native-lib' library on application startup.
    static
    {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        edit_thread_count = findViewById(R.id.edit_thread_count);
        edit_iter_count = findViewById(R.id.edit_iter_count);
        btn_start_thread = findViewById(R.id.btn_start_thread);
        tv_log = findViewById(R.id.tv_log);

        nativeInit();
        btn_start_thread.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                int threads = getNumber(edit_thread_count, 0);
                int iterations = getNumber(edit_iter_count, 0);

                if (threads > 0 && iterations > 0) {
                    startThreads(threads, iterations);
                }
            }
        });
    }

    @Override
    protected void onDestroy()
    {
        nativeFree();
        super.onDestroy();
    }

    private void onNativeMessage(final String message) {
        runOnUiThread(new Runnable() {
            public void run() {
                tv_log.append(message);
                tv_log.append("\n");
            }
        });
    }

    private static int getNumber(EditText editText, int defaultValue) {
        int value;
        try {
            value = Integer.parseInt(editText.getText().toString());
        } catch (NumberFormatException e) {
            value = defaultValue;
        }

        return value;
    }

    private void startThreads(int threads, int iterations) {
        posixThreads(threads, iterations);
    }

    public native String stringFromJNI();

    /**
     * Initializes the native code.
     */
    public native void nativeInit();

    /**
     * Free the native resources.
     */
    public native void nativeFree();

    /**
     * Native worker.
     * @param id worker id.
     * @param iterations iteration count.
     */
    public native void nativeWorker(int id, int iterations);

    /**
     * Using the POSIX threads.
     *
     * @param threads thread count.
     * @param iterations iteration count.
     */
    public native void posixThreads(int threads, int iterations);

    /**
     * Using Java based threads.
     * @param threads thread count.
     * @param iterations iteration count.
     */
    public void javaThreads(int threads, final int iterations) {
        // Create a Java based thread for each worker
        for (int i = 0; i < threads; i++) {
            final int id = i;

            Thread thread = new Thread() {
                public void run() {
                    nativeWorker(id, iterations);
                }
            };

            thread.start();
        }
    }

}
