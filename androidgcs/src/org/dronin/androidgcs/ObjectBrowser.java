/**
 ******************************************************************************
 * @file       ObjectBrowser.java
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      A simple object browser for UAVOs that allows viewing, editing,
 *             loading and saving.
 * @see        The GNU Public License (GPL) Version 3
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */
package org.dronin.androidgcs;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.ListIterator;

import org.dronin.androidgcs.drawer.NavDrawerActivityConfiguration;
import org.dronin.androidgcs.fragments.ObjectEditor;
import org.dronin.androidgcs.fragments.ObjectViewer;
import org.dronin.uavtalk.UAVDataObject;
import org.dronin.uavtalk.UAVObject;

import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ListView;

public class ObjectBrowser extends ObjectManagerActivity 
	implements OnSharedPreferenceChangeListener {

	private final String TAG = "ObjectBrower";
	int selected_index = -1;
	boolean connected;
	SharedPreferences prefs;
	ArrayAdapter<UAVDataObject> adapter;
	List<UAVDataObject> allObjects;

	enum DisplayMode {NONE, VIEW, EDIT};
	DisplayMode displayMode = DisplayMode.NONE;
	
	/**
	 * Display the fragment to edit this object
	 * @param id
	 */
	public void editObject(int id) {
		
		Log.d(TAG, "editObject("+id+")");

		displayMode = DisplayMode.EDIT;
		
		Bundle b = new Bundle();
		b.putString("org.dronin.androidgcs.ObjectName", allObjects.get(selected_index).getName());
		b.putLong("org.dronin.androidgcs.ObjectId", allObjects.get(selected_index).getObjID());
		b.putLong("org.dronin.androidgcs.InstId", allObjects.get(selected_index).getInstID());

		Fragment newFrag = new ObjectEditor();
		newFrag.setArguments(b);
		
		FragmentTransaction trans = getFragmentManager().beginTransaction();
		trans.replace(R.id.object_information, newFrag);
		trans.addToBackStack(null);
		trans.commit();

	}
	
	/**
	 * Display the fragment to view this object
	 * @param id
	 */
	public void viewObject(int id) {
		Bundle b = new Bundle();
		b.putString("org.dronin.androidgcs.ObjectName", allObjects.get(selected_index).getName());
		b.putLong("org.dronin.androidgcs.ObjectId", allObjects.get(selected_index).getObjID());
		b.putLong("org.dronin.androidgcs.InstId", allObjects.get(selected_index).getInstID());

		Fragment newFrag = new ObjectViewer();
		newFrag.setArguments(b);
		
		// If currently editing something, remove that from the
		// back stack during horizontal navigation
		if (displayMode == DisplayMode.EDIT)
			getFragmentManager().popBackStack();

		FragmentTransaction trans = getFragmentManager().beginTransaction();
		trans.replace(R.id.object_information, newFrag);
		trans.commit();
		
		displayMode = DisplayMode.VIEW;
	}
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		prefs = PreferenceManager.getDefaultSharedPreferences(this);
		prefs.registerOnSharedPreferenceChangeListener(this);

		 ((CheckBox) findViewById(R.id.dataCheck)).setChecked(prefs.getBoolean("browser_show_data",true));
		 ((CheckBox) findViewById(R.id.settingsCheck)).setChecked(prefs.getBoolean("browser_show_settings",true));
		 
		 if (savedInstanceState != null) {
				displayMode = (DisplayMode) savedInstanceState.getSerializable("org.dronin.browser.mode");
				selected_index = savedInstanceState.getInt("org.dronin.browser.selected");
		 }
	}
	
	@Override
	protected NavDrawerActivityConfiguration getNavDrawerConfiguration() {
		NavDrawerActivityConfiguration navDrawer = getDefaultNavDrawerConfiguration();
		navDrawer.setMainLayout(R.layout.object_browser);
		return navDrawer;
	}
	
	@Override
	void onConnected() {
		super.onConnected();

		OnCheckedChangeListener checkListener = new OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView,
					boolean isChecked) {
				prefs = PreferenceManager.getDefaultSharedPreferences(ObjectBrowser.this);
				Editor editor = prefs.edit();
				Log.d(TAG, "Writing settings");
				editor.putBoolean("browser_show_data", ((CheckBox) findViewById(R.id.dataCheck)).isChecked());
				editor.putBoolean("browser_show_settings", ((CheckBox) findViewById(R.id.settingsCheck)).isChecked());
				editor.commit();
			}
		};

		((CheckBox) findViewById(R.id.dataCheck)).setOnCheckedChangeListener(checkListener);
		((CheckBox) findViewById(R.id.settingsCheck)).setOnCheckedChangeListener(checkListener);

		updateList();
	}

	public void attachObjectView() {
		Log.d(TAG, "attachObjectView()");
		((Button) findViewById(R.id.editButton)).setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (selected_index > 0) {
					editObject(selected_index);
				}
			}
		});

		((Button) findViewById(R.id.object_load_button)).setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				UAVObject objPer = objMngr.getObject("ObjectPersistence");

				if (selected_index > 0 && objPer != null) {
					objPer.getField("Operation").setValue("Load");
					try {
						objPer.getField("Selection").setValue("SingleObject");
					} catch (NullPointerException e) {
						// No longer used.
					}

					Log.d(TAG,"Loading with object id: " + allObjects.get(selected_index).getObjID());
					objPer.getField("ObjectID").setValue(allObjects.get(selected_index).getObjID());
					objPer.getField("InstanceID").setValue(0);
					objPer.updated();

					allObjects.get(selected_index).updateRequested();
				}
			}
		});
	}

	/**
	 * Populate the list of UAVO objects based on the selected filter
	 */
	private void updateList() {

		boolean includeData = ((CheckBox) findViewById(R.id.dataCheck)).isChecked();
		boolean includeSettings = ((CheckBox) findViewById(R.id.settingsCheck)).isChecked();

		List<List<UAVDataObject>> allobjects = objMngr.getDataObjects();
		allObjects = new ArrayList<UAVDataObject>();
		ListIterator<List<UAVDataObject>> li = allobjects.listIterator();
		while(li.hasNext()) {
			List<UAVDataObject> objects = li.next();
			if(includeSettings && objects.get(0).isSettings())
				allObjects.addAll(objects);
			else if (includeData && !objects.get(0).isSettings())
				allObjects.addAll(objects);
		}

		Collections.sort(allObjects, new Comparator<UAVDataObject>() {
			@Override
			public int compare(UAVDataObject obj1, UAVDataObject obj2)
			{
				if (obj1.isSettings() && (!obj2.isSettings())) {
					// Put settings before data.
					return -1;
				}

				if (obj2.isSettings() && (!obj1.isSettings())) {
					return 1;
				}

				// Otherwise sort by name
				return obj1.getName().compareTo(obj2.getName());
			}
		});

		ListView objects = (ListView) findViewById(R.id.object_list);
		adapter = new ArrayAdapter<UAVDataObject>(this, R.layout.object_browser_item, allObjects);
		objects.setAdapter(adapter);		
		objects.setOnItemClickListener(new OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {
				
				selected_index = position;
				viewObject(selected_index);
			}
		});
		
		if (selected_index >= 0) {
			objects.setSelection(selected_index);
			objects.setItemChecked(selected_index, true);
		}
	}


	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
			String key) {
		Log.d(TAG, "Settings updated");
		if (key.equals("browser_show_data")) {
			((CheckBox) findViewById(R.id.dataCheck)).setChecked(prefs.getBoolean("browser_show_data",true));
			updateList();
		}
		if (key.equals("browser_show_settings")) {
			((CheckBox) findViewById(R.id.settingsCheck)).setChecked(prefs.getBoolean("browser_show_settings",true));
			updateList();
		}
	}
	
	@Override
	public void onSaveInstanceState (Bundle outState) {
		super.onSaveInstanceState(outState);
		
		outState.putSerializable("org.dronin.browser.mode", displayMode);
		outState.putInt("org.dronin.browser.selected", selected_index);
	}
}
