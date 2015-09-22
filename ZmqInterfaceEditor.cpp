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
            
            const String item (items [row]);
            bool enabled = false;
            
        } // end of function
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