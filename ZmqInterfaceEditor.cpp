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
private ListBoxModel, public AsyncUpdater
{
public:
    ZmqInterfaceEditorListBox(const String noItemsText, ZmqInterfaceEditor *e):
    ListBox(String::empty, nullptr), noItemsMessage(noItemsText)
    {
        editor = e;
        setModel(this);
        
        backgroundGradient = ColourGradient(Colour(190, 190, 190), 0.0f, 0.0f,
                                            Colour(185, 185, 185), 0.0f, 120.0f, false);
        backgroundGradient.addColour(0.2f, Colour(155, 155, 155));
        
        backgroundColor = Colour(155, 155, 155);
        
        setColour(backgroundColourId, backgroundColor);
        
        refresh();

    }
    
    void handleAsyncUpdate()
    {
        refresh();
    }
    
    void refresh()
    {
        updateContent();
        repaint();
        
    }
    
    int getNumRows() override
    {
        return editor->getApplicationList()->size();
    }
    
    
    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        OwnedArray<ZmqApplication> *items = editor->getApplicationList();
        if (isPositiveAndBelow (row, items->size()))
        {
            g.fillAll(Colour(155, 155, 155));
            if (rowIsSelected)
                g.fillAll (findColour (TextEditor::highlightColourId)
                           .withMultipliedAlpha (0.3f));
            
            ZmqApplication *i = (*items)[row];
            const String item (i->name); // TODO change when we put a map
            bool enabled = false;
            
            const int x = getTickX();
            
            g.setFont (height * 0.6f);
            if(i->alive)
                g.setColour(Colours::green);
            else
                g.setColour(Colours::red);
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
        g.setColour (Colours::grey);
        g.setGradientFill(backgroundGradient);
        if (editor->getApplicationList()->size() == 0)
        {
            g.setColour (Colours::grey);
            g.setFont (13.0f);
            g.drawText (noItemsMessage,
                        0, 0, getWidth(), getHeight() / 2,
                        Justification::centred, true);
        }
    }
    
    
    
    
private:
    const String noItemsMessage;
    ZmqInterfaceEditor *editor;
    /** Stores the editor's background color. */
    Colour backgroundColor;
    
    /** Stores the editor's background gradient. */
    ColourGradient backgroundGradient;
    
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
    listBox->setBounds(2,25,130,105);
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

void ZmqInterfaceEditor::refreshListAsync()
{
    listBox->triggerAsyncUpdate();
}

OwnedArray<ZmqApplication> *ZmqInterfaceEditor::getApplicationList()
{
    OwnedArray<ZmqApplication> *ar = ZmqProcessor->getApplicationList();
    return ar;
    
}

