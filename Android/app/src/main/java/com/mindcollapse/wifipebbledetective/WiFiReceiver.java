package com.mindcollapse.wifipebbledetective;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;

import java.util.List;

// Interface for callback
interface WiFiReceiverCallback {
    void scanResultsReceived(List<ScanResult> networks);
}

// WiFi scan results receiver
public class WiFiReceiver extends BroadcastReceiver {
    Context context;
    WifiManager wifiManager;
    WiFiReceiverCallback callback;

    // OnReceive handler
    @Override
    public void onReceive(Context c, Intent intent) {
        if (intent.getAction().equals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)) {
            callback.scanResultsReceived(wifiManager.getScanResults());
        }
    }

    // Register the receiver
    public void Register() {
        context.registerReceiver(this, new IntentFilter(
            WifiManager.SCAN_RESULTS_AVAILABLE_ACTION
        ));
    }

    // Initiate WiFi scanning
    public void Scan() {
        wifiManager.startScan();
    }

    // Unregister the receiver
    public void Unregister() {
        context.unregisterReceiver(this);
    }

    // Constructor
    public WiFiReceiver(Context context, WiFiReceiverCallback callback)
    {
        wifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);

        this.callback = callback;
        this.context = context;
    }
}
