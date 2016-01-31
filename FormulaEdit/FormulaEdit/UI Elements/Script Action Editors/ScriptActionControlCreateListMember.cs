﻿using System.Windows.Forms;

namespace FormulaEdit.UI_Elements.Script_Action_Editors
{
    public partial class ScriptActionControlCreateListMember : UserControl
    {
        internal ScriptActionControlCreateListMember(MudData.FormulaActionCreateListMember action)
        {
            InitializeComponent();

            ListComboBox.Text = action.list;
            ArchetypeComboBox.Text = action.archetype;

            foreach (var kvp in action.@params)
            {
                ParamDataGridView.Rows.Add(new object[] { kvp.Key, kvp.Value });
            }
        }
    }
}