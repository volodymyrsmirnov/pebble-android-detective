package com.mindcollapse.wifipebbledetective;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.ScanResult;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.CompoundButton;
import android.widget.ToggleButton;

import java.util.LinkedList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.ArrayBlockingQueue;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.util.PebbleDictionary;

public class WiFiDetective extends Activity implements WiFiReceiverCallback {
    private WiFiReceiver receiver;

    // If pebble is connected and Detective is enables
    private boolean isActive = false;

    private List<String> knownPublicNetworks = new LinkedList<String>();

    // Pebble applucatuin UUID
    private final static UUID PEBBLE_APP_UUID = UUID.fromString("2db008d5-535c-4daf-a92a-0a30b32080cd");

    // Message type constants
    private final static int WFD_TYPE = 0;
    private final static int WFD_ENCRYPTED = 2;
    private final static int WFD_QUALITY = 3;
    private final static int WFD_SSID = 4;
    private final static int WFD_BSSID = 5;
    private final static int WFD_VIBE = 6;

    // Buffer to overcome the problem of busy Pebble for simultaneous data sending
    private ArrayBlockingQueue<PebbleDictionary> buffer = new ArrayBlockingQueue<PebbleDictionary>(16);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_wi_fi_detective);

        receiver = new WiFiReceiver(getApplicationContext(), this);
        receiver.Register();

        // Toggle button logic
        final ToggleButton toggle = (ToggleButton) findViewById(R.id.toggle);
        toggle.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                isActive = b;

                if (!PebbleKit.isWatchConnected(getApplicationContext()))
                {
                    isActive = false;
                    toggle.setChecked(false);

                    return;
                }

                if (isActive) {
                    PebbleKit.startAppOnPebble(getApplicationContext(), PEBBLE_APP_UUID);
                    receiver.Scan();
                }
                else
                {
                    PebbleKit.closeAppOnPebble(getApplicationContext(), PEBBLE_APP_UUID);
                }
            }
        });

        // Poll data from buffer queue on every ACK response
        PebbleKit.registerReceivedAckHandler(getApplicationContext(), new PebbleKit.PebbleAckReceiver(PEBBLE_APP_UUID) {
            @Override
            public void receiveAck(Context context, int transactionId) {
                PebbleDictionary data = buffer.poll();

                if (data != null)
                {
                    PebbleKit.sendDataToPebble(getApplicationContext(), PEBBLE_APP_UUID, data);
                }
                else if (isActive)
                {
                    receiver.Scan();
                }
            }
        });
    }

    @Override
    public void scanResultsReceived(List<ScanResult> networks) {
        if (!isActive)
            return;

        buffer.clear();

        // Cleanup signal
        PebbleDictionary cleanup = new PebbleDictionary();
        cleanup.addInt8(WFD_TYPE, (byte)0);
        buffer.add(cleanup);

        // Iterate over network
        for(ScanResult network : networks)
        {
            PebbleDictionary data = new PebbleDictionary();

            byte encrypted = 0;

            // Dummy way to detect encryption from the network capabilities
            for (String encryptedMode : new String[] { "WEP", "PSK", "EAP" })
            {
                if (network.capabilities.contains(encryptedMode))
                {
                    encrypted = 1;
                    break;
                }
            }

            // Vibrate if unknown public network been discovered
            if (encrypted == 0 && !knownPublicNetworks.contains(network.SSID)) {
                data.addInt8(WFD_VIBE, (byte)1);
                knownPublicNetworks.add(network.SSID);
            }
            else {
                data.addInt8(WFD_VIBE, (byte)0);
            }

            data.addInt8(WFD_TYPE, (byte) 1);
            data.addInt8(WFD_ENCRYPTED, encrypted);
            data.addInt8(WFD_QUALITY, (byte)(2 * (network.level + 100)));
            data.addString(WFD_SSID, network.SSID);
            data.addString(WFD_BSSID, network.BSSID);

            buffer.offer(data);
        }

        // Send cleanup message to initiate sending
        PebbleKit.sendDataToPebble(getApplicationContext(), PEBBLE_APP_UUID, buffer.poll());
    }
}
