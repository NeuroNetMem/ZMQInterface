/*
 ------------------------------------------------------------------
 
 This file is part of the Open Ephys GUI
 Copyright (C) 2015 Open Ephys
 
 ------------------------------------------------------------------
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */

/*
  ==============================================================================

    ZmqInterfaceEditor.cpp
    Created: 22 Sep 2015 9:42:44am
    Author:  Francesco Battaglia

  ==============================================================================
*/




#include "ZmqInterfaceEditor.h"
#include "ZmqInterface.h"

class ZmqInterfaceEditor::ZmqInterfaceEditorListBox: public ListBox,
private ListBoxModel
{
public:
    ZmqInterfaceEditorListBox(const String noItemsText, ZmqInterfaceEditor *e):
    ListBox(String::empty, nullptr), noItemsMessage(noItemsText)
    {
        refresh();
        setModel(this);
        editor = e;
    }
    
    void refresh()
    {
        items.clear();
        items = editor->getApplicationList();
        updateContent();
        repaint();
        
    }
    
    int getNumRows() override
    {
        return items.size();
    }
    
    
    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (isPositiveAndBelow (row, items.size()))
        {
            if (rowIsSelected)
                g.fillAll (findColour (TextEditor::highlightColourId)
                           .withMultipliedAlpha (0.3f));
            
            const String item (items [row]); // TODO change when we put a map
            bool enabled = false;
            
            const int x = getTickX();
            const float tickW = height * 0.75f;
            
            
            getLookAndFeel().drawTickBox (g, *this, x - tickW, (height - tickW) / 2, tickW, tickW, enabled, true, true, false);
            
            g.setFont (height * 0.6f);
            g.setColour (findColour (ListBox::textColourId, true).withMultipliedAlpha (enabled ? 1.0f : 0.6f));
            g.drawText (item, x, 0, width - x - 2, height, Justification::centredLeft, true);
        } // end of function
    }
    
    
    void listBoxItemClicked (int row, const MouseEvent& e) override
    {
        selectRow (row);
    }
    
    void paint (Graphics& g) override
    {
        ListBox::paint (g);
        
        if (items.size() == 0)
        {
            g.setColour (Colours::grey);
            g.setFont (13.0f);
            g.drawText (noItemsMessage,
                        0, 0, getWidth(), getHeight() / 2,
                        Justification::centred, true);
        }
    }
    
    
    
    
private:
    StringArray items;
    const String noItemsMessage;
    ZmqInterfaceEditor *editor;
    
    
    int getTickX() const
    {
        return getRowHeight() + 5;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZmqInterfaceEditorListBox)
};

ZmqInterfaceEditor::ZmqInterfaceEditor(GenericProcessor *parentNode, bool useDefaultParameters): GenericEditor(parentNode, useDefaultParameters)
{
    ZmqProcessor = (ZmqInterface *)parentNode;
    listBox = new ZmqInterfaceEditorListBox(String("no app connected"), this);
    listBox->setBounds(20,80,140,150);
    addAndMakeVisible(listBox);
    setEnabledState(false);
    
}

ZmqInterfaceEditor::~ZmqInterfaceEditor()
{
    deleteAllChildren();
}

void ZmqInterfaceEditor::saveCustomParameters(XmlElement *xml)
{
    
}

void ZmqInterfaceEditor::loadCustomParameters(XmlElement* xml)
{
    
}


StringArray ZmqInterfaceEditor::getApplicationList()
{
    int i = 0;
    StringArray ar = ZmqProcessor->getApplicationList();
    return ar;
    
}

