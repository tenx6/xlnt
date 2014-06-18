#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <unordered_map>

#include <pugixml.hpp>

#include "writer/writer.hpp"
#include "cell/cell.hpp"
#include "constants.hpp"
#include "worksheet/range.hpp"
#include "worksheet/range_reference.hpp"
#include "worksheet/worksheet.hpp"
#include "workbook/workbook.hpp"
#include "common/relationship.hpp"

namespace xlnt {

std::string writer::write_shared_strings(const std::vector<std::string> &string_table)
{
    pugi::xml_document doc;
    auto root_node = doc.append_child("sst");
    root_node.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
    root_node.append_attribute("uniqueCount").set_value((int)string_table.size());
    
    for(auto string : string_table)
    {
        root_node.append_child("si").append_child("t").text().set(string.c_str());
    }
    
    std::stringstream ss;
    doc.save(ss);
    
    return ss.str();
}

std::string writer::write_properties_core(const document_properties &/*prop*/)
{
    return "";
}

std::string writer::write_properties_app(const workbook &/*wb*/)
{
    return "";
}

std::string writer::write_workbook_rels(const workbook &wb)
{
    return write_relationships(wb.get_relationships());
}

std::string writer::write_worksheet_rels(worksheet ws)
{
	return write_relationships(ws.get_relationships());
}


std::string writer::write_workbook(const workbook &wb)
{
    pugi::xml_document doc;
    auto root_node = doc.append_child("workbook");
    root_node.append_attribute("xmlns").set_value("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
    root_node.append_attribute("xmlns:r").set_value("http://schemas.openxmlformats.org/officeDocument/2006/relationships");
    
    auto file_version_node = root_node.append_child("fileVersion");
    file_version_node.append_attribute("appName").set_value("xl");
    file_version_node.append_attribute("lastEdited").set_value("4");
    file_version_node.append_attribute("lowestEdited").set_value("4");
    file_version_node.append_attribute("rupBuild").set_value("4505");
    
    auto workbook_pr_node = root_node.append_child("workbookPr");
    workbook_pr_node.append_attribute("codeName").set_value("ThisWorkbook");
    workbook_pr_node.append_attribute("defaultThemeVersion").set_value("124226");
    
    auto book_views_node = root_node.append_child("bookViews");
    auto workbook_view_node = book_views_node.append_child("workbookView");
    workbook_view_node.append_attribute("activeTab").set_value("0");
    workbook_view_node.append_attribute("autoFilterDateGrouping").set_value("1");
    workbook_view_node.append_attribute("firstSheet").set_value("0");
    workbook_view_node.append_attribute("minimized").set_value("0");
    workbook_view_node.append_attribute("showHorizontalScroll").set_value("1");
    workbook_view_node.append_attribute("showSheetTabs").set_value("1");
    workbook_view_node.append_attribute("showVerticalScroll").set_value("1");
    workbook_view_node.append_attribute("tabRatio").set_value("600");
    workbook_view_node.append_attribute("visibility").set_value("visible");
    
    auto sheets_node = root_node.append_child("sheets");
    auto defined_names_node = root_node.append_child("definedNames");

    int i = 0;
    for(auto ws : wb)
    {
        auto sheet_node = sheets_node.append_child("sheet");
        sheet_node.append_attribute("name").set_value(ws.get_title().c_str());
        sheet_node.append_attribute("r:id").set_value((std::string("rId") + std::to_string(i + 1)).c_str());
        sheet_node.append_attribute("sheetId").set_value(std::to_string(i + 1).c_str());

        if(ws.has_auto_filter())
        {
            auto defined_name_node = defined_names_node.append_child("definedName");
            defined_name_node.append_attribute("name").set_value("_xlnm._FilterDatabase");
            defined_name_node.append_attribute("hidden").set_value(1);
            defined_name_node.append_attribute("localSheetId").set_value(0);
            std::string name = "'" + ws.get_title() + "'!" + range_reference::make_absolute(ws.get_auto_filter()).to_string();
            defined_name_node.text().set(name.c_str());
        }

        i++;
    }
    
    auto calc_pr_node = root_node.append_child("calcPr");
    calc_pr_node.append_attribute("calcId").set_value("124519");
    calc_pr_node.append_attribute("calcMode").set_value("auto");
    calc_pr_node.append_attribute("fullCalcOnLoad").set_value("1");
    
    std::stringstream ss;
    doc.save(ss);
    
    return ss.str();
}

std::string writer::write_worksheet(worksheet ws, const std::vector<std::string> &string_table, const std::unordered_map<std::size_t, std::string> &style_id_by_hash)
{
    ws.get_cell("A1");

    pugi::xml_document doc;
    auto root_node = doc.append_child("worksheet");
    root_node.append_attribute("xmlns").set_value(constants::Namespaces.at("spreadsheetml").c_str());
    root_node.append_attribute("xmlns:r").set_value(constants::Namespaces.at("r").c_str());
    auto sheet_pr_node = root_node.append_child("sheetPr");
    auto outline_pr_node = sheet_pr_node.append_child("outlinePr");
    outline_pr_node.append_attribute("summaryBelow").set_value(1);
    outline_pr_node.append_attribute("summaryRight").set_value(1);
    auto dimension_node = root_node.append_child("dimension");
    dimension_node.append_attribute("ref").set_value(ws.calculate_dimension().to_string().c_str());
    auto sheet_views_node = root_node.append_child("sheetViews");
    auto sheet_view_node = sheet_views_node.append_child("sheetView");
    sheet_view_node.append_attribute("workbookViewId").set_value(0);
    
    std::string active_pane = "bottomRight";

    if(ws.has_frozen_panes())
    {
        auto pane_node = sheet_view_node.append_child("pane");

        if(ws.get_frozen_panes().get_column_index() > 0)
        {
            pane_node.append_attribute("xSplit").set_value(ws.get_frozen_panes().get_column_index());
            active_pane = "topRight";
        }

        if(ws.get_frozen_panes().get_row_index() > 0)
        {
            pane_node.append_attribute("ySplit").set_value(ws.get_frozen_panes().get_row_index());
            active_pane = "bottomLeft";
        }

        if(ws.get_frozen_panes().get_row_index() > 0 && ws.get_frozen_panes().get_column_index() > 0)
        {
            auto top_right_node = sheet_view_node.append_child("selection");
            top_right_node.append_attribute("pane").set_value("topRight");
            auto bottom_left_node = sheet_view_node.append_child("selection");
            bottom_left_node.append_attribute("pane").set_value("bottomLeft");
            active_pane = "bottomRight";
        }

        pane_node.append_attribute("topLeftCell").set_value(ws.get_frozen_panes().to_string().c_str());
        pane_node.append_attribute("activePane").set_value(active_pane.c_str());
        pane_node.append_attribute("state").set_value("frozen");
    }

    auto selection_node = sheet_view_node.append_child("selection");
    if(ws.has_frozen_panes())
    {
        if(ws.get_frozen_panes().get_row_index() > 0 && ws.get_frozen_panes().get_column_index() > 0)
        {
            selection_node.append_attribute("pane").set_value("bottomRight");
        }
        else if(ws.get_frozen_panes().get_row_index() > 0)
        {
            selection_node.append_attribute("pane").set_value("bottomLeft");
        }
        else if(ws.get_frozen_panes().get_column_index() > 0)
        {
            selection_node.append_attribute("pane").set_value("topRight");
        }
    }
    std::string active_cell = "A1";
    selection_node.append_attribute("activeCell").set_value(active_cell.c_str());
    selection_node.append_attribute("sqref").set_value(active_cell.c_str());
    
    auto sheet_format_pr_node = root_node.append_child("sheetFormatPr");
    sheet_format_pr_node.append_attribute("baseColWidth").set_value(10);
    sheet_format_pr_node.append_attribute("defaultRowHeight").set_value(15);
    
    std::vector<int> styled_columns;
    
    if(!style_id_by_hash.empty())
    {
        for(auto row : ws.rows())
        {
            for(auto cell : row)
            {
                if(cell.has_style())
                {
                    styled_columns.push_back(xlnt::cell_reference::column_index_from_string(cell.get_column()));
                }
            }
        }
        
        auto cols_node = root_node.append_child("cols");
        std::sort(styled_columns.begin(), styled_columns.end());
        for(auto column : styled_columns)
        {
            auto col_node = cols_node.append_child("col");
            col_node.append_attribute("min").set_value(column);
            col_node.append_attribute("max").set_value(column);
            col_node.append_attribute("style").set_value(1);
        }
    }

    std::unordered_map<std::string, std::string> hyperlink_references;
    
    auto sheet_data_node = root_node.append_child("sheetData");
    for(auto row : ws.rows())
    {
        row_t min = (int)row.num_cells();
        row_t max = 0;
        bool any_non_null = false;
        
        for(auto cell : row)
        {
            min = std::min(min, cell_reference::column_index_from_string(cell.get_column()));
            max = std::max(max, cell_reference::column_index_from_string(cell.get_column()));
            
            if(cell.get_data_type() != cell::type::null)
            {
                any_non_null = true;
            }
        }
        
        if(!any_non_null)
        {
            continue;
        }
        
        auto row_node = sheet_data_node.append_child("row");
        row_node.append_attribute("r").set_value(row.front().get_row());
        
        row_node.append_attribute("spans").set_value((std::to_string(min) + ":" + std::to_string(max)).c_str());
        if(ws.has_row_properties(row.front().get_row()))
        {
            row_node.append_attribute("customHeight").set_value(1);
            auto height = ws.get_row_properties(row.front().get_row()).height;
            if(height == std::floor(height))
            {
                row_node.append_attribute("ht").set_value((std::to_string((int)height) + ".0").c_str());
            }
            else
            {
                row_node.append_attribute("ht").set_value(height);
            }
        }
        //row_node.append_attribute("x14ac:dyDescent").set_value(0.25);
        
        for(auto cell : row)
        {
            if(cell.get_data_type() != cell::type::null || cell.is_merged())
            {
                if(cell.has_hyperlink())
                {
                    hyperlink_references[cell.get_hyperlink().get_id()] = cell.get_reference().to_string();
                }

                auto cell_node = row_node.append_child("c");
                cell_node.append_attribute("r").set_value(cell.get_reference().to_string().c_str());
                
                if(cell.get_data_type() == cell::type::string)
                {
                    int match_index = -1;
                    for(int i = 0; i < (int)string_table.size(); i++)
                    {
                        if(string_table[i] == cell.get_internal_value_string())
                        {
                            match_index = i;
                            break;
                        }
                    }
                    
                    if(match_index == -1)
                    {
                        cell_node.append_attribute("t").set_value("inlineStr");
                        auto inline_string_node = cell_node.append_child("is");
                        inline_string_node.append_child("t").text().set(cell.get_internal_value_string().c_str());
                    }
                    else
                    {
                        cell_node.append_attribute("t").set_value("s");
                        auto value_node = cell_node.append_child("v");
                        value_node.text().set(match_index);
                    }
                }
                else
                {
                    if(cell.get_data_type() != cell::type::null)
                    {
                        if(cell.get_data_type() == cell::type::boolean)
                        {
                            cell_node.append_attribute("t").set_value("b");
                            auto value_node = cell_node.append_child("v");
                            value_node.text().set(cell.get_internal_value_numeric() == 0 ? 0 : 1);
                        }
                        else if(cell.get_data_type() == cell::type::numeric)
                        {
                            cell_node.append_attribute("t").set_value("n");
                            auto value_node = cell_node.append_child("v");
                            if(std::floor(cell.get_internal_value_numeric()) == cell.get_internal_value_numeric())
                            {
                                value_node.text().set((long long)cell.get_internal_value_numeric());
                            }
                            else
                            {
                                value_node.text().set((double)cell.get_internal_value_numeric());
                            }
                        }
                        else if(cell.get_data_type() == cell::type::formula)
                        {
                            cell_node.append_child("f").text().set(cell.get_internal_value_string().substr(1).c_str());
                            cell_node.append_child("v");
                        }
                    }
                }
                
                if(cell.has_style())
                {
                    cell_node.append_attribute("s").set_value(1);
                }
            }
        }
    }

    if(ws.has_auto_filter())
    {
        auto auto_filter_node = root_node.append_child("autoFilter");
        auto_filter_node.append_attribute("ref").set_value(ws.get_auto_filter().to_string().c_str());
    }
    
    if(!ws.get_merged_ranges().empty())
    {
        auto merge_cells_node = root_node.append_child("mergeCells");
        merge_cells_node.append_attribute("count").set_value((unsigned int)ws.get_merged_ranges().size());
        
        for(auto merged_range : ws.get_merged_ranges())
        {
            auto merge_cell_node = merge_cells_node.append_child("mergeCell");
            merge_cell_node.append_attribute("ref").set_value(merged_range.to_string().c_str());
        }
    }

    if(!ws.get_relationships().empty())
    {
        auto hyperlinks_node = root_node.append_child("hyperlinks");

        for(auto relationship : ws.get_relationships())
        {
            auto hyperlink_node = hyperlinks_node.append_child("hyperlink");
            hyperlink_node.append_attribute("display").set_value(relationship.get_target_uri().c_str());
            hyperlink_node.append_attribute("ref").set_value(hyperlink_references.at(relationship.get_id()).c_str());
            hyperlink_node.append_attribute("r:id").set_value(relationship.get_id().c_str());
        }
    }
    
    auto page_margins_node = root_node.append_child("pageMargins");
        
    page_margins_node.append_attribute("left").set_value(ws.get_page_margins().get_left());
    page_margins_node.append_attribute("right").set_value(ws.get_page_margins().get_right());
    page_margins_node.append_attribute("top").set_value(ws.get_page_margins().get_top());
    page_margins_node.append_attribute("bottom").set_value(ws.get_page_margins().get_bottom());
    page_margins_node.append_attribute("header").set_value(ws.get_page_margins().get_header());
    page_margins_node.append_attribute("footer").set_value(ws.get_page_margins().get_footer());
    
    if(!ws.get_page_setup().is_default())
    {
        auto page_setup_node = root_node.append_child("pageSetup");
        
        std::string orientation_string = ws.get_page_setup().get_orientation() == page_setup::orientation::landscape ? "landscape" : "portrait";
        page_setup_node.append_attribute("orientation").set_value(orientation_string.c_str());
        page_setup_node.append_attribute("paperSize").set_value((int)ws.get_page_setup().get_paper_size());
        page_setup_node.append_attribute("fitToHeight").set_value(ws.get_page_setup().fit_to_height() ? 1 : 0);
        page_setup_node.append_attribute("fitToWidth").set_value(ws.get_page_setup().fit_to_width() ? 1 : 0);
        
        auto page_set_up_pr_node = root_node.append_child("pageSetUpPr");
        page_set_up_pr_node.append_attribute("fitToPage").set_value(ws.get_page_setup().fit_to_page() ? 1 : 0);
    }
    
    std::stringstream ss;
    doc.save(ss);
    
    return ss.str();
}
    
std::string writer::write_content_types(const workbook &wb)
{
    pugi::xml_document doc;
    auto root_node = doc.append_child("Types");
    root_node.append_attribute("xmlns").set_value(constants::Namespaces.at("content-types").c_str());
    
    for(auto type : wb.get_content_types())
    {
		pugi::xml_node type_node;

		if (type.is_default)
		{
			type_node = root_node.append_child("Default");
			type_node.append_attribute("Extension").set_value(type.extension.c_str());
		}
		else
		{
			type_node = root_node.append_child("Override");
			type_node.append_attribute("PartName").set_value(type.part_name.c_str());
		}

		type_node.append_attribute("ContentType").set_value(type.type.c_str());
    }
    
    std::stringstream ss;
    doc.save(ss);
    return ss.str();
}

std::string xlnt::writer::write_root_rels()
{
	std::vector<relationship> relationships;

	relationships.push_back(relationship("a"));
	relationships.push_back(relationship("b"));
	relationships.push_back(relationship("c"));

	return write_relationships(relationships);
}

std::string writer::write_relationships(const std::vector<relationship> &relationships)
{
    pugi::xml_document doc;
    auto root_node = doc.append_child("Relationships");
    root_node.append_attribute("xmlns").set_value(constants::Namespaces.at("relationships").c_str());
    
    for(auto relationship : relationships)
    {
        auto app_props_node = root_node.append_child("Relationship");
        app_props_node.append_attribute("Id").set_value(relationship.get_id().c_str());
        app_props_node.append_attribute("Target").set_value(relationship.get_target_uri().c_str());
        app_props_node.append_attribute("Type").set_value(relationship.get_target_uri().c_str());
    }
    
    std::stringstream ss;
    doc.save(ss);
    return ss.str();
}
    
std::string writer::write_theme()
{
     pugi::xml_document doc;
     auto theme_node = doc.append_child("a:theme");
     theme_node.append_attribute("xmlns:a").set_value(constants::Namespaces.at("drawingml").c_str());
     theme_node.append_attribute("name").set_value("Office Theme");
     auto theme_elements_node = theme_node.append_child("a:themeElements");
     auto clr_scheme_node = theme_elements_node.append_child("a:clrScheme");
     clr_scheme_node.append_attribute("name").set_value("Office");
     
     struct scheme_element
     {
         std::string name;
         std::string sub_element_name;
         std::string val;
     };
     
     std::vector<scheme_element> scheme_elements =
     {
         {"a:dk1", "a:sysClr", "windowText"},
         {"a:lt1", "a:sysClr", "windowText"},
         {"a:dk2", "a:srgbClr", "1F497D"},
         {"a:lt2", "a:srgbClr", "EEECE1"},
         {"a:accent1", "a:srgbClr", "4F81BD"},
         {"a:accent2", "a:srgbClr", "C0504D"},
         {"a:accent3", "a:srgbClr", "9BBB59"},
         {"a:accent4", "a:srgbClr", "8064A2"},
         {"a:accent5", "a:srgbClr", "4BACC6"},
         {"a:accent6", "a:srgbClr", "F79646"},
         {"a:hlink", "a:srgbClr", "0000FF"},
         {"a:folHlink", "a:srgbClr", "800080"},
     };
     
     for(auto element : scheme_elements)
     {
         auto element_node = clr_scheme_node.append_child(element.name.c_str());
         element_node.append_child(element.sub_element_name.c_str()).append_attribute("val").set_value(element.val.c_str());

         if(element.name == "a:dk1")
         {
             element_node.append_attribute("lastClr").set_value("000000");
         }
         else if(element.name == "a:lt1")
         {
             element_node.append_attribute("lastClr").set_value("FFFFFF");
         }
     }

     struct font_scheme
     {
         bool typeface;
         std::string script;
         std::string major;
         std::string minor;
     };

     std::vector<font_scheme> font_schemes = 
     {
         {true, "a:latin", "Cambria", "Calibri"},
         {true, "a:ea", "", ""},
         {true, "a:cs", "", ""},
         {false, "Jpan", "&#xFF2D;&#xFF33; &#xFF30;&#x30B4;&#x30B7;&#x30C3;&#x30AF;", "&#xFF2D;&#xFF33; &#xFF30;&#x30B4;&#x30B7;&#x30C3;&#x30AF;"},
         {false, "Hang", "&#xB9D1;&#xC740; &#xACE0;&#xB515;", "&#xB9D1;&#xC740; &#xACE0;&#xB515;"},
         {false, "Hans", "&#x5B8B;&#x4F53", "&#x5B8B;&#x4F53"},
         {false, "Hant", "&#x65B0;&#x7D30;&#x660E;&#x9AD4;", "&#x65B0;&#x7D30;&#x660E;&#x9AD4;"},
         {false, "Arab", "Times New Roman", "Arial"},
         {false, "Hebr", "Times New Roman", "Arial"},
         {false, "Thai", "Tahoma", "Tahoma"},
         {false, "Ethi", "Nyala", "Nyala"},
         {false, "Beng", "Vrinda", "Vrinda"},
         {false, "Gujr", "Shruti", "Shruti"},
         {false, "Khmr", "MoolBoran", "DaunPenh"},
         {false, "Knda", "Tunga", "Tunga"},
         {false, "Guru", "Raavi", "Raavi"},
         {false, "Cans", "Euphemia", "Euphemia"},
         {false, "Cher", "Plantagenet Cherokee", "Plantagenet Cherokee"},
         {false, "Yiii", "Microsoft Yi Baiti", "Microsoft Yi Baiti"},
         {false, "Tibt", "Microsoft Himalaya", "Microsoft Himalaya"},
         {false, "Thaa", "MV Boli", "MV Boli"},
         {false, "Deva", "Mangal", "Mangal"},
         {false, "Telu", "Guatami", "Guatami"},
         {false, "Taml", "Latha", "Latha"},
         {false, "Syrc", "Estrangelo Edessa", "Estrangelo Edessa"},
         {false, "Orya", "Kalinga", "Kalinga"},
         {false, "Mlym", "Kartika", "Kartika"},
         {false, "Laoo", "DokChampa", "DokChampa"},
         {false, "Sinh", "Iskoola Pota", "Iskoola Pota"},
         {false, "Mong", "Mongolian Baiti", "Mongolian Baiti"},
         {false, "Viet", "Times New Roman", "Arial"},
         {false, "Uigh", "Microsoft Uighur", "Microsoft Uighur"}
     };

     auto font_scheme_node = theme_elements_node.append_child("a:fontScheme");
     font_scheme_node.append_attribute("name").set_value("Office");

     auto major_fonts_node = font_scheme_node.append_child("a:majorFont");
     auto minor_fonts_node = font_scheme_node.append_child("a:minorFont");

     for(auto scheme : font_schemes)
     {
         pugi::xml_node major_font_node, minor_font_node;

         if(scheme.typeface)
         {
             major_font_node = major_fonts_node.append_child(scheme.script.c_str());
             minor_font_node = minor_fonts_node.append_child(scheme.script.c_str());
         }
         else
         {
             major_font_node = major_fonts_node.append_child("a:font");
             major_font_node.append_attribute("script").set_value(scheme.script.c_str());
             minor_font_node = minor_fonts_node.append_child("a:font");
             minor_font_node.append_attribute("typeface").set_value(scheme.script.c_str());
         }

         major_font_node.append_attribute("typeface").set_value(scheme.major.c_str());
         minor_font_node.append_attribute("typeface").set_value(scheme.minor.c_str());
     }

     auto format_scheme_node = theme_elements_node.append_child("a:fmtScheme");
     format_scheme_node.append_attribute("name").set_value("Office");

     auto fill_style_list_node = format_scheme_node.append_child("a:fillStyleList");
     fill_style_list_node.append_child("a:solidFill").append_child("a:schemeClr").append_attribute("val").set_value("phClr");

     /*auto line_style_list_node = */format_scheme_node.append_child("a:lnStyleList");
     /*auto effect_style_list_node = */format_scheme_node.append_child("a:effectStyleList");
     /*auto bg_fill_style_list_node = */format_scheme_node.append_child("a:bgFillStyleList");

     theme_node.append_child("a:objectDefaults");
     theme_node.append_child("a:extraClrSchemeLst");
     
     std::stringstream ss;
     doc.print(ss);
     return ss.str();
}

}
