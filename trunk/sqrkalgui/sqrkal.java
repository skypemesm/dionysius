import java.io.*;
import java.util.*;
import java.awt.* ;
import java.awt.event.* ;
import javax.swing.* ;
import javax.swing.ImageIcon;
import javax.swing.border.Border;
import javax.swing.border.LineBorder;
import javax.swing.BorderFactory;


/**
	This class represents the GUI for sqrkal. It in turn calls the c++ program sqrkald (sqrkal daemon)
**/

class sqrkal extends JPanel implements ActionListener{
	
 //Controls used
 private JPanel contentPane;
 private JLabel jLabel1,jLabel2,jLabel3,jLabel4,jlO;
 private JButton b1,b2;
 private ImageIcon mainLabelIcon;
 public static String[] optionsStrings = { "Regular","Full Bandwidth", "With Burst Padding", "With Gradual Ascent Algorithm"};
 private JComboBox optionsList;
 public static final String COPYRIGHT  = "\u00a9";

 public sqrkal() {

		ImageIcon mainLabelIcon = createImageIcon("images/a.png");

		//CREATE AND ADD THE LABELS
		jLabel1 = new JLabel("",mainLabelIcon,SwingConstants.CENTER);
		jLabel2 = new JLabel("  Securing your VoIP calls against traffic analysis attacks.  ");
		jLabel3 = new JLabel("---------------------------------------------------------------------------------------------");
		
		jLabel1.setFont(new Font("Serif", Font.BOLD, 26));
		jLabel1.setBackground(new Color(204,204,153));
		jLabel1.setForeground(Color.WHITE);
		jLabel1.setOpaque(true);

		jLabel2.setFont(new Font("Times New Roman", Font.PLAIN, 12));
		jLabel2.setBackground(new Color(204,204,153));
		jLabel2.setForeground(new Color(54,53,17));
		jLabel2.setOpaque(true);

		add(jLabel1);
		add(jLabel2);
		add(jLabel3);

		//CREATE AND ADD OPTIONS LABEL
		jlO = new JLabel("Select OPTIONS:");
		jlO.setFont(new Font("Serif", Font.BOLD, 14));
		jlO.setBackground(new Color(204,204,153));
		jlO.setForeground(new Color(54,53,17));
		jlO.setOpaque(true);
		add(jlO);

		

		//CREATE AND ADD OPTIONS LIST
		optionsList = new JComboBox(optionsStrings);
		ComboBoxRenderer renderer = new ComboBoxRenderer();
		optionsList.setOpaque(true);
		optionsList.setRenderer(renderer);
		optionsList.setSelectedIndex(0);
		optionsList.repaint();
		optionsList.addActionListener(this);
		add(optionsList);

		add(new JSeparator(SwingConstants.HORIZONTAL));

		//ADD THE START AND STOP BUTTONS
		Border raisedBorder = BorderFactory.createRaisedBevelBorder();
		
		b1 = new JButton("START");
		b1.setFont(new Font("Serif", Font.BOLD, 26));
		b1.setBackground(new Color(102,102,51));
		b1.setForeground(new Color(204,153,0));
		b1.setBorder(raisedBorder);
		b1.setVerticalTextPosition(AbstractButton.CENTER);
		b1.setHorizontalTextPosition(AbstractButton.CENTER);
		b1.setPreferredSize(new Dimension(220,100));
		b1.setMnemonic(KeyEvent.VK_S);


		add(new JSeparator(SwingConstants.HORIZONTAL));

		b2 = new JButton("STOP");
		b2.setBorder(raisedBorder);
		b2.setFont(new Font("Serif", Font.BOLD, 26));
		b2.setBackground(new Color(102,102,51));
		b2.setForeground(new Color(204,153,0));

		b2.setPreferredSize(new Dimension(222,100));
		b2.setVerticalTextPosition(AbstractButton.CENTER);
		b2.setHorizontalTextPosition(AbstractButton.CENTER);
		b2.setMnemonic(KeyEvent.VK_T);
		b2.setEnabled(false);

		//Listen for actions on buttons 1 and 2.
		b1.addActionListener(this);
		b2.addActionListener(this);

		b1.setToolTipText("Click this button to START SQRKal.");
		b2.setToolTipText("Click this button to STOP SQRKal.");

		add(b1);
		add(b2);

		add(new JSeparator(SwingConstants.HORIZONTAL));

		//CREATE AND ADD COPYRIGHT LABEL
		jLabel4 = new JLabel("Copyright"+ COPYRIGHT +" Texas A&M University.");
		jLabel4.setFont(new Font("Serif", Font.ITALIC, 10));
		jLabel4.setBackground(Color.CYAN);
		    
		add(jLabel4);
		setBackground(new Color(204,204,153));

	  }


 /** Returns an ImageIcon, or null if the path was invalid. */
    protected static ImageIcon createImageIcon(String path) {
        java.net.URL imgURL = sqrkal.class.getResource(path);
        if (imgURL != null) {
            return new ImageIcon(imgURL);
        } else {
            System.err.println("Couldn't find file: " + path);
            return null;
        }
    }

/** ACTION LISTENER HANDLER **/
 public void actionPerformed(ActionEvent e) {

  String cmd = "sudo skill -9 sqrkald";	
	if (e.getSource() == b1 )
	{
		cmd="sudo ./runme.sh ";
		
		switch(optionsList.getSelectedIndex())
		{
		  case 0:
			break;

		  case 1:
			cmd += "-fb";break;
		  case 2:
			cmd += "-bp";break;
		  case 3:	
			cmd += "-ga";break;

		}
		b1.setEnabled(false);
		b2.setEnabled(true);
		optionsList.setEnabled(false);
		//System.out.println(cmd);

	} else if (e.getSource() == b2 ){
		//System.out.println(cmd);

		b2.setEnabled(false);
		b1.setEnabled(true);
		optionsList.setEnabled(true);

	} else if (e.getSource() == optionsList ){
	}

	try
        {             
            Runtime rt = Runtime.getRuntime();
            Process proc = rt.exec(cmd);

           // int exitVal = proc.exitValue();
            //System.out.println("ExitValue: " + exitVal);        

        } catch (Throwable t)
          {
            t.printStackTrace();
          }


	
 }


/** MAIN function **/
 public static void main(String args[])
 {
	System.out.println("SQRKal starting up...");

     	//Create and set up the window.
        JFrame frame = new JFrame("SQRKal v0.2.0");
	frame.getContentPane().setBackground(Color.BLACK);
	frame.setSize(398,480); 
	frame.setLocation(300,200);
	frame.setResizable(false);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);


	UIManager.put("ComboBox.selectionBackground", new Color(254,253,0));
	UIManager.put("ComboBox.buttonBackground", new Color(254,253,0));
	UIManager.put("ComboBox.background", new Color(254,253,0));
	UIManager.put("Button.disabledText",new Color(51,51,0));
	UIManager.put("ComboBox.disabledBackground",new Color(51,51,0));
        //Create and set up the content pane.
        sqrkal newContentPane = new sqrkal();
        frame.setContentPane(newContentPane);

        //Display the window.
        frame.setVisible(true);
	
	
    }
}


/** List Cell renderer for options List combobox **/
class ComboBoxRenderer extends JLabel
                       implements ListCellRenderer {

    public ComboBoxRenderer() {
        setOpaque(true);
        setHorizontalAlignment(CENTER);
        setVerticalAlignment(CENTER);
    }

    /*
     * This method finds the image and text corresponding
     * to the selected value and returns the label, set up
     * to display the text and image.
     */
    public Component getListCellRendererComponent(
                                       JList list,
                                       Object value,
                                       int index,
                                       boolean isSelected,
                                       boolean cellHasFocus) {

	if (isSelected) {
            setBackground(new Color(254,253,0));
            setForeground(list.getSelectionForeground());
        } else {
            setBackground(new Color(204,204,153));
            setForeground(list.getForeground());
        }

	setText((String)value);

        setFont(list.getFont());
        	
        return this;
    }
}

