package org.chromium.android_webview.shell;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashSet;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.google.zxing.integration.android.IntentIntegrator;
import com.google.zxing.integration.android.IntentResult;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;
import android.net.Uri;

import com.google.vr.ndk.base.DaydreamApi;

public class URLEntryActivity extends Activity
{
	private static final String LAST_USED_URL_KEY = "url";
	private static final String CONFIG_KEY = "chromiumConfig";
	private static final String URL_HISTORY_URLS_KEY = "urlHistoryURLs";
	
	private EditText urlEditText = null;
	private Button removeSelectedURLsButton = null;
	private Button clearURLHistoryButton = null;
	private CheckBox clearCacheCheckBox = null;
	private boolean clearCacheFromConfigFile = false;
	private boolean clearCacheFromConfigFileWarningShown = false;
	private URLHistoryListViewAdapter urlHistoryListViewAdapter = null;
	private JSONArray urlHistoryURLs = null;
	private JSONObject config = null;
	private HashSet<Integer> urlIndicesToRemove = new HashSet<Integer>();

	private DaydreamApi ddApi = null;

	private OnCheckedChangeListener checkedChangeListener = new OnCheckedChangeListener()
	{
		@Override
		public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
		{
			int position = (Integer)buttonView.getTag();
			if (isChecked)
			{
				urlIndicesToRemove.add(position);
			}
			else {
				urlIndicesToRemove.remove(position);
			}
			boolean enabled = !urlIndicesToRemove.isEmpty();
			removeSelectedURLsButton.setEnabled(enabled);
		}
	};
	private OnClickListener urlHistoryListViewEntryTextViewClicked = new OnClickListener()
	{
		@Override
		public void onClick(View view)
		{
			TextView textView = (TextView)view;
			urlEditText.setText(textView.getText());
		}
	}; 

	private void addDefaultURLsToURLHistoryIfTheyDoNotExist() throws JSONException
	{
		String[] defaultURLs = {
			"http://threejs.org/examples/webgl_shader2.html",
			"https://aframe.io/aframe/examples/boilerplate-helloworld/",
			"http://threejs.org/examples/webgl_materials_cubemap.html",
			"http://threejs.org/examples/webgl_loader_collada.html",
			"http://threejs.org/examples/webgl_morphtargets_horse.html",
			"https://vizor.io/edelblut/space-adventure",
			"http://threejs.org/examples/webgl_materials_grass.html",
			"http://threejs.org/examples/webgl_lights_pointlights.html",
			"http://threejs.org/examples/webgl_shadowmesh.html",
			"file:///android_asset/tests/basic/index01.html",
			"file:///android_asset/tests/basic/index02.html",
			"file:///android_asset/tests/video/index.html?vrwebglvideo",
			"file:///android_asset/tests/cubes/index.html?fullwebvrapi"
		};
		for (int i = 0; i < defaultURLs.length; i++)
		{
			addURLToHistoryIfItDoesNotExist(defaultURLs[i]);
		}
	}
	
	private void saveStringToPreferences(String name, String value) 
	{
		Editor editor = getPreferences(Activity.MODE_PRIVATE).edit();
		editor.putString(name, value);
		if (!editor.commit())
		{
	  		AlertDialog alertDialog = Utils.createAlertDialog(URLEntryActivity.this, "Error saving URL", "For an unknown reason, the URL or URL history could not be saved to the app preferences. The information might not be available from one execution to another if you close the app.", null, 1, "Ok", null, null);
	  		alertDialog.show();
		}
	}	
	
	private void addURLToHistoryIfItDoesNotExist(String url) throws JSONException
	{
		boolean exists = false;
		for (int i = 0; !exists && i < urlHistoryURLs.length(); i++)
		{
			exists = urlHistoryURLs.getString(i).equals(url);
		}
		if (!exists)
		{
			urlHistoryURLs.put(url);
			urlHistoryListViewAdapter.add(url);
			urlHistoryListViewAdapter.notifyDataSetChanged();	
			saveStringToPreferences(URL_HISTORY_URLS_KEY, urlHistoryURLs.toString());
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		ddApi = DaydreamApi.create(this);
		
		setContentView(R.layout.url_entry_layout);
		
		ImageButton qrcodeImageButton = (ImageButton)this.findViewById(R.id.qrcodeImageButton);
		qrcodeImageButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				IntentIntegrator intentIntegrator = new IntentIntegrator(URLEntryActivity.this);
				intentIntegrator.initiateScan();
			}
		});

		Button goButton = (Button)this.findViewById(R.id.goButton);
		goButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
		  	String url = urlEditText.getText().toString();
		  	try
		  	{
		  		new URL(url);
		  		try
		  		{
		  			addURLToHistoryIfItDoesNotExist(url);
		  		}
		  		catch(JSONException e)
		  		{
		  			Utils.createAlertDialog(URLEntryActivity.this, "JSON Exception", "Could not add the url to the url history. " + e.getCause() + " - " + e.getMessage(), null, 1, "Ok", null, null).show();
		  		}
		  		saveStringToPreferences(LAST_USED_URL_KEY, url);
		  		clearURLHistoryButton.setEnabled(urlHistoryURLs.length() > 0);
					Intent intent = new Intent(getApplicationContext(), AwShellActivity.class);
					intent.setData(Uri.parse(url));
					intent.putExtra("config", config.toString());
					ddApi.launchInVr(intent);
					// startActivity(intent);
		  	}
		  	catch(MalformedURLException e)
		  	{
		  		Utils.createAlertDialog(URLEntryActivity.this, "Not an URL", "The text does not represent a valid URL.", null, 1, "Ok", null, null).show();
		  	}
			}
		});
		
		removeSelectedURLsButton = (Button)this.findViewById(R.id.removeSelectedURLsButton);
		removeSelectedURLsButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				if (urlIndicesToRemove.isEmpty()) return;
				// As there is no remove function in JSONArrays until API 19:
				// 1.- make a copy of the JSONArray without the elements to be removed and storing the removed urls
				// 2.- recreate the JSONArray from the copy without the removed urls
				// 3.- remove the removed urls from the listview adapter
				try
				{
					int length = urlHistoryURLs.length();
					ArrayList<String> urlHistoryURLsCopyWithoutRemovedURLs = new ArrayList<String>(length - urlIndicesToRemove.size());
					ArrayList<String> removedURLs = new ArrayList<String>(urlIndicesToRemove.size());
					for (int i = 0; i < length; i++)
					{
						String url = urlHistoryURLs.getString(i); 
						if (urlIndicesToRemove.contains(i))
						{
							removedURLs.add(url);
						}
						else 
						{
							urlHistoryURLsCopyWithoutRemovedURLs.add(url);
						}
					}
					urlHistoryURLs = new JSONArray(urlHistoryURLsCopyWithoutRemovedURLs);
					for (String url: removedURLs)
					{
						urlHistoryListViewAdapter.remove(url);
					}
					urlHistoryListViewAdapter.notifyDataSetChanged();					
					saveStringToPreferences(URL_HISTORY_URLS_KEY, urlHistoryURLs.toString());
					clearURLHistoryButton.setEnabled(urlHistoryURLs.length() > 0);
				}
				catch(JSONException e)
				{
					Utils.createAlertDialog(URLEntryActivity.this, "JSON Exception", "Could not correctly get a URL string from the JSONArray in order to reove it: " + e.getCause() + " - " + e.getMessage(), null, 1, "Ok", null, null).show();
				}
			}
		});
		
		clearURLHistoryButton = (Button)this.findViewById(R.id.clearURLHistoryButton);
		clearURLHistoryButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				Utils.createAlertDialog(URLEntryActivity.this, "Clear URL History", "Are you sure you want to clear the URL history with '" + urlHistoryURLs.length() + "' entries?", new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface dialog, int which)
					{
						switch(which)
						{
						case AlertDialog.BUTTON_POSITIVE:
							urlHistoryURLs = new JSONArray();
							urlHistoryListViewAdapter.clear();
							urlHistoryListViewAdapter.notifyDataSetChanged();					
				  		saveStringToPreferences(URL_HISTORY_URLS_KEY, urlHistoryURLs.toString());
				  		clearURLHistoryButton.setEnabled(urlHistoryURLs.length() > 0);
							break;
						}
					}
				}, 2, "Yes", "No", null).show();
			}
		});

		// This is protocol to be followed:
		// 0.- If a URL is passed in the intent, use it directly.
		// 1.- Read if the config.json file exists in the assets.
		// 		1.1.- If it exists, check if the clearCache property exists.
		//			1.1.1.- If the property exists, it's value is what drives the checkBox and if the user changes the value he/she needs to be notified that the values will only be used in the execution but it won't be stored as the assets file value prevails.
		//			1.1.2.- If the property does not exist then go to 1.2
		//		1.2.- If it does not exist, then read the sharedpreferences for the configuration. There will be an initial json by default.
		//			1.2.1.- If the clearCache property exists, use it and store it in the sharedpreferences everytime the user changes its value.
		String configString = null;

		// Try to read the config from the received intent (if there is one)
	    Intent receivedIntent = getIntent();
	    if (receivedIntent != null)
	    {
	    	Bundle receivedExtras = receivedIntent.getExtras();
	    	if (receivedExtras != null)
	    	{
	    		configString = receivedExtras.getString("config");
	    	}
	    }

	    // If there is no config string, try to read it from the assets or from the preferences.
		boolean configFileRead = false;
		if (configString == null)
		{
			try
			{
				configString = Utils.readFromAssets(this, "config.json");
				configFileRead = true;
			}
			catch(IOException e) 
			{
				System.out.println("WARNING: IOException reading 'config.json'. Using preferences instead.");
				configString = getPreferences(Activity.MODE_PRIVATE).getString(CONFIG_KEY, "{}");
			}
		}

		String url = null;
		try
		{
			config = new JSONObject(configString);

			// If the config information contains the active skipURLEntryActivity flag, then skip this activity.
			if (config.has("skipURLEntryActivity") && config.getBoolean("skipURLEntryActivity")) {
				Intent intent = new Intent(getApplicationContext(), AwShellActivity.class);
				if (url != null)
				{
					intent.setData(Uri.parse(url));
				}
				intent.putExtra("config", config.toString());
				startActivity(intent);
				return;
			}

			clearCacheFromConfigFile = configFileRead && config.has("clearCache");
			clearCacheCheckBox = (CheckBox)this.findViewById(R.id.clearCacheCheckBox);
			clearCacheCheckBox.setChecked(config.has("clearCache") ? config.getBoolean("clearCache") : true);
			config.put("clearCache", clearCacheCheckBox.isChecked());
			saveStringToPreferences(CONFIG_KEY, config.toString());
			url = config.has("url") ? config.getString("url") : null;

			clearCacheCheckBox.setOnCheckedChangeListener(new OnCheckedChangeListener()
			{
				@Override
				public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
				{
					if (clearCacheFromConfigFile && !clearCacheFromConfigFileWarningShown)
					{
						Utils.createAlertDialog(URLEntryActivity.this, "WARNING", "The current clear cache flag has been read from the config.json asset file. Your selection will be used in this execution but it won't be stored while the config.json asset file is provided.", null, 1, "Ok", null, null).show();
						clearCacheFromConfigFileWarningShown = true;
					}
					try
					{
						config.put("clearCache", isChecked);
						saveStringToPreferences(CONFIG_KEY, config.toString());
					}
					catch(JSONException e)
					{
						Utils.createAlertDialog(URLEntryActivity.this, "JSON Exception", "Could not correctly store the clearCache flag in the config JSON object: " + e.getCause() + " - " + e.getMessage(), null, 1, "Ok", null, null).show();
					}
				}
			});
		}
		catch(JSONException e)
		{
			Utils.createAlertDialog(this, "JSON Exception", "Could not correctly initialize the config: " + e.getCause() + " - " + e.getMessage(), null, 1, "Ok", null, null).show();
		}
		
		urlEditText = (EditText)this.findViewById(R.id.urlEditText);
		urlEditText.setText(url != null ? url : getPreferences(Activity.MODE_PRIVATE).getString(LAST_USED_URL_KEY, ""));
		
		urlHistoryListViewAdapter = new URLHistoryListViewAdapter();
		ListView urlHistoryListView = (ListView)this.findViewById(R.id.urlHistoryListView); 
		urlHistoryListView.setAdapter(urlHistoryListViewAdapter);
		try
		{
			String urlHistoryURlsJSONString = getPreferences(Activity.MODE_PRIVATE).getString(URL_HISTORY_URLS_KEY, "[]");
			urlHistoryURLs = new JSONArray(urlHistoryURlsJSONString);
			int length = urlHistoryURLs.length();
			for (int i = 0; i < length; i++)
			{
				urlHistoryListViewAdapter.add(urlHistoryURLs.getString(i));
			}

			addDefaultURLsToURLHistoryIfTheyDoNotExist();
			
			clearURLHistoryButton.setEnabled(length > 0);
		}
		catch (JSONException e)
		{
			Utils.createAlertDialog(this, "JSON Exception", "Could not correctly parse and load previous URL history in JSON format: " + e.getCause() + " - " + e.getMessage(), null, 1, "Ok", null, null).show();
		}

	}
	
	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent intent) 
	{
	  IntentResult scanResult = IntentIntegrator.parseActivityResult(requestCode, resultCode, intent);
	  if (scanResult != null && scanResult.getContents() != null) 
	  {
	  	String url = scanResult.getContents();
	  	try
	  	{
	  		new URL(url);
	  		urlEditText.setText(url);
	  	}
	  	catch(MalformedURLException e)
	  	{
	  		AlertDialog alertDialog = Utils.createAlertDialog(this, "Not an URL", "The read QRCode does not represent a valid URL.", null, 1, "Ok", null, null);
	  		alertDialog.show();
	  	}
	  }
	}
	
	private class URLHistoryListViewAdapter extends ArrayAdapter<String> 
	{
		public URLHistoryListViewAdapter()
		{
			super(URLEntryActivity.this, R.layout.url_history_list_item_layout);
		}

		@Override
		public View getView(int position, View view, ViewGroup parent) 
		{
			if (view == null)
			{
				LayoutInflater inflater = getLayoutInflater();
				view = inflater.inflate(R.layout.url_history_list_item_layout, parent, false); 
			}
			try
			{
				TextView urlHistoryListViewEntryTextView = (TextView) view.findViewById(R.id.urlHistoryEntryTextView);
				urlHistoryListViewEntryTextView.setOnClickListener(urlHistoryListViewEntryTextViewClicked);
				urlHistoryListViewEntryTextView.setText(urlHistoryURLs.getString(position));
				CheckBox urlHistoryListViewEntryCheckBox = (CheckBox)view.findViewById(R.id.urlHistoryEntryCheckbox);
				urlHistoryListViewEntryCheckBox.setTag(position);
				urlHistoryListViewEntryCheckBox.setChecked(false);
				urlHistoryListViewEntryCheckBox.setOnCheckedChangeListener(checkedChangeListener);
			}
			catch(JSONException e)
			{
				Utils.createAlertDialog(this.getContext(), "JSON Exception", "JSONException accesing element at index '" + position + "' of the URL History JSON array. " + e.getCause() + " - " + e.getMessage(), null, 1, "Ok", null, null).show();
			}
			return view;
		}
	}
}