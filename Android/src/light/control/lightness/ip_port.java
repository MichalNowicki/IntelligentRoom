package light.control.lightness;

import java.net.InetAddress;

public class ip_port {
	public InetAddress IP;
	public int PORT;

	public InetAddress getIP() {
		return IP;
	}

	public void setIP(InetAddress iP) {
		IP = iP;
	}

	public int getPORT() {
		return PORT;
	}

	public void setPORT(int pORT) {
		PORT = pORT;
	}

	ip_port(InetAddress ipaddress, int i) {
		IP = ipaddress;
		PORT = i;
	}

}
