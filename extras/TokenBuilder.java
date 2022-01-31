package es.upm.dit.tokenbuilder;


import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.UnknownHostException;
import java.util.Base64;
import java.util.function.Consumer;

import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTError;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.LocationEvent;
import org.eclipse.swt.browser.LocationListener;
import org.eclipse.swt.browser.ProgressEvent;
import org.eclipse.swt.browser.ProgressListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;

// import es.upm.dit.moodle.evaluation.server.curl.JCurl;

public class TokenBuilder {

    private final String NET_URL = "https://moodle.upm.es/titulaciones/oficiales";
    private String url = "";
    private boolean moodleAvailable=false;
    enum EstadoBrowser {NONE,Login,CAS,Logged};
    private EstadoBrowser browserState=EstadoBrowser.NONE;
    private String moodleToken=null;
	private Display display;
	private Label urlLabel;
	private Shell shell;
	private Browser browser;

    public TokenBuilder() {
        this.display = new Display();
        display.setRuntimeExceptionHandler(new Consumer<RuntimeException>() {
			@Override
			public void accept(RuntimeException t) {
				System.out.println("Se ha producido una excepción en la ejecución de la ventana "+t.getMessage());
			}
        });
        display.setErrorHandler(new Consumer<Error>() {
			@Override
			public void accept(Error t) {
				System.out.println("Se ha pproducido un error en la JVM "+t.getMessage());
			}
        });
        shell = new Shell(display);
        FormLayout formLayout = new FormLayout();
        shell.setLayout(formLayout);

        try {
        	if (SWT.getVersion() > 5100)
        		browser = new Browser(shell, 0x40000); // SWT.EDGE);
        	else
        		browser = new Browser(shell, SWT.NONE);
            browser.addLocationListener(new LocationListener() {
            	@Override
				public void changed(LocationEvent arg0) {
					System.out.println("LocationListener::changed "+arg0.location);
					urlLabel.setText(arg0.location);
					if ("https://moodle.upm.es/titulaciones/oficiales/login/auth_index.php".equals(arg0.location)) {
						browserState=EstadoBrowser.Login;
						System.out.println("Estamos haciendo el login de moodle");
					}
					if ("https://moodle.upm.es/titulaciones/oficiales/login/index.php?authCAS=CAS".equals(arg0.location)) {
						browserState=EstadoBrowser.CAS;
						System.out.println("Estamos haciendo el login en CAS");
					}
					if ("https://moodle.upm.es/titulaciones/oficiales/my/".equals(arg0.location)) {
						browserState=EstadoBrowser.Logged;
						System.out.println("Está hecho el login de moodle");
						browser.setUrl("https://moodle.upm.es/titulaciones/oficiales/admin/tool/mobile/launch.php?service=moodle_mobile_app&passport=12345&urlscheme=tokenbuilder");
					}
					if (browserState == EstadoBrowser.Logged && "https://moodle.upm.es/titulaciones/oficiales/login/login.php".equals(arg0.location)) {
						System.out.println("Está hecho el logout de moodle");
						display.dispose();
					}						
				}
            	@Override
				public void changing(LocationEvent arg0) {
					System.out.println("LocationListener::changing "+arg0.location);
					if (arg0.location.startsWith("tokenbuilder://token=")) {
						int end = arg0.location.indexOf("==");
						String token=arg0.location.substring("tokenbuilder://token=".length(), end);
						String base64Decoded=new String(Base64.getDecoder().decode(token));
						moodleToken=base64Decoded.split(":::")[1];
						System.out.println("Podemos acceder a moodle. Hacemos el loggout ");//+moodleToken);
						browser.setUrl("https://moodle.upm.es/titulaciones/oficiales/login/login.php");
					}
				}
            	
            });
            browser.addProgressListener(new ProgressListener() {
				@Override
				public void changed(ProgressEvent arg0) {
					System.out.println("Estamos descargando página web ");
				}

				@Override
				public void completed(ProgressEvent arg0) {
					System.out.println("Progress completed "+arg0.toString());
					browser.getParent().layout();
				}
            });
        } catch (SWTError e) {
            System.err.println("No se puede instanciar Browser: " + e.getMessage()+". Tu instalación de eclipse no permite acceder a moodle");
            e.printStackTrace();
            display.dispose();
            moodleAvailable=false;
            return;
        }
        FormData data = new FormData();
        final Composite composite = new Composite(shell, SWT.NONE);
        FormLayout compLayout = new FormLayout();
        composite.setLayout(compLayout);
		data.height = 40;
		data.width = 700;
		data.top = new FormAttachment(0,0);
		composite.setLayoutData(data);
		final Button itemRefresh = new Button(composite, SWT.PUSH);
		itemRefresh.setText("Refresca");
		itemRefresh.setSelection(true);
		urlLabel=new Label(composite, SWT.BORDER);
		urlLabel.setText(browser.getUrl());
		data = new FormData();
		data.height = 40;
		data.width = 650;
		data.left = new FormAttachment(itemRefresh, 5, SWT.DEFAULT);
		urlLabel.setLayoutData(data);
		
        data = new FormData();
		data.left = new FormAttachment(0, 0);
		data.bottom = new FormAttachment(100,0);
		data.right = new FormAttachment(100, 0);
		data.top = new FormAttachment(composite, 5, SWT.DEFAULT);
		browser.setLayoutData(data);
        
		Listener listener = new Listener() {
			public void handleEvent(Event event) {
				browser.refresh();
			}
		};
		itemRefresh.addListener(SWT.Selection, listener);

    }
    
    private boolean isInternetReachable() {
        HttpURLConnection urlConnect = null;

        try {
            // make a URL to a known source
            URL url = new URL(NET_URL);
            // open a connection to that source
            urlConnect = (HttpURLConnection) url.openConnection();
            // trying to retrieve data from the source. If there is no connection, this line will fail
            urlConnect.getContent();
        } catch (UnknownHostException e) {
            return false;
        } catch (IOException e) {
            return false;
        } finally {
            // cleanup
            if(urlConnect != null) urlConnect.disconnect();
        }
        return true;
    }

    public boolean moodleAvailable() {
    	return moodleAvailable;
    }
    
    public void start() {
        shell.open();

        if(isInternetReachable()) 
        	browser.setUrl(NET_URL);
        else {
        	System.err.println("No está accesible "+NET_URL+": o el servidor no está accesible o no hay acceso a internet");
        	moodleAvailable=false;
        	display.dispose();
        	return;
        }

        moodleAvailable=true;
        
        while (!shell.isDisposed()) {
            if (!display.readAndDispatch())
                display.sleep();
        }
        display.dispose();
    }
    static {
    	String os=getOS();
    	if (os.equals("osx")) {
    		try {
        		Class atsc = Class.forName("org.eclipse.swt.internal.cocoa.NSDictionary");
        		Class nsstring = Class.forName("org.eclipse.swt.internal.cocoa.NSString");
        		Class cid = Class.forName("org.eclipse.swt.internal.cocoa.id");
        		Method mdictionaryWithObject = atsc.getDeclaredMethod("dictionaryWithObject", cid,cid);
				Object arg1 = Class.forName("org.eclipse.swt.internal.cocoa.NSNumber").getDeclaredMethod("numberWithBool", Boolean.TYPE).invoke(null, true);
				Object arg2 = Class.forName("org.eclipse.swt.internal.cocoa.NSString").getDeclaredMethod("stringWith", String.class).invoke(null, "NSAllowsArbitraryLoads");
				Object ats = mdictionaryWithObject.invoke(null,arg1,arg2);
				Class cnsBundle=Class.forName("org.eclipse.swt.internal.cocoa.NSBundle");
				Object mainBundle = cnsBundle.getDeclaredMethod("mainBundle").invoke(null);
				Object diccio = cnsBundle.getMethod("infoDictionary").invoke(mainBundle);
				Object arg3 = Class.forName("org.eclipse.swt.internal.cocoa.NSString").getDeclaredMethod("stringWith", String.class).invoke(null, "NSAppTransportSecurity");
				Class.forName("org.eclipse.swt.internal.cocoa.NSObject").getMethod("setValue", cid, nsstring).invoke(diccio, ats, arg3);
			} catch (Exception e) {
				// e.printStackTrace();
			} 
    		/*
    		org.eclipse.swt.internal.cocoa.NSDictionary ats = org.eclipse.swt.internal.cocoa.NSDictionary.dictionaryWithObject(
	 	           org.eclipse.swt.internal.cocoa.NSNumber.numberWithBool(true),
	 	           org.eclipse.swt.internal.cocoa.NSString.stringWith("NSAllowsArbitraryLoads"));
	 	   org.eclipse.swt.internal.cocoa.NSBundle.mainBundle().infoDictionary().setValue(
	 	           ats, org.eclipse.swt.internal.cocoa.NSString.stringWith("NSAppTransportSecurity"));
	 	   */
    	}
    }
    
    private static String getOS() {
 	   String osNameProperty = System.getProperty("os.name"); 

 	   if (osNameProperty == null) {
 	       System.err.println("os.name property no está actulizado"); 
 	       return null;
 	   } 
 	   else 
 	       osNameProperty = osNameProperty.toLowerCase(); 

 	   if (osNameProperty.contains("win")) 
 	       return "win"; 
 	   else if (osNameProperty.contains("mac")) 
 	       return "osx"; 
 	   else if (osNameProperty.contains("linux") || osNameProperty.contains("nix")) 
 	       return "linux"; 
 	   else 
 	   { 
 	       System.err.println("SO desconocido " + osNameProperty); 
 	       return null;
 	   } 
    }
    
    public String getMoodleToken() {
    	return moodleToken;
    }
	
    public static void main(String[] args) {
        new TokenBuilder().start();
    }
}
