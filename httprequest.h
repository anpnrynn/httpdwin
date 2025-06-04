//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#ifndef HTTPREQUEST_H_INCLUDED
#define HTTPREQUEST_H_INCLUDED

#include <iostream>
#include <map>

using namespace std;


class NameMimeValues {
    public:
        string m_Name;
        string m_Mime;
        string m_Value;
        string m_TempFileName;
};

class HttpRequest {
    private:
    public:

        string m_Method;
        string m_Version;
        string m_DecodedUrl;
        string m_EncodedUrl;

        string m_TempPostFileName;
        string m_TempPutFileName;

        string m_Accept;
        string m_Accept_CH;
        string m_Accept_Encoding;
        string m_Accept_Language;
        string m_Accept_Patch;
        string m_Accept_Post;
        string m_Accept_Ranges;
        string m_Access_Control_Allow_Credentials;
        string m_Access_Control_Allow_Headers;
        string m_Access_Control_Allow_Methods;
        string m_Access_Control_Allow_Origin;
        string m_Access_Control_Expose_Headers;
        string m_Access_Control_Max_Age;
        string m_Access_Control_Request_Headers;
        string m_Access_Control_Request_Method;
        string m_Age;
        string m_Allow;
        string m_Alt_Svc;
        string m_Alt_Used;
        string m_Attribution_Reporting_EligibleExperimental;
        string m_Attribution_Reporting_Register_SourceExperimental;
        string m_Attribution_Reporting_Register_TriggerExperimental;
        string m_Authorization;
        string m_Available_DictionaryExperimental;
        string m_Cache_Control;
        string m_Clear_Site_Data;
        string m_Connection;
        string m_Content_Digest;
        string m_Content_Disposition;
        string m_Content_DPRNon_standardDeprecated;
        string m_Content_Encoding;
        string m_Content_Language;
        string m_Content_Length;
        string m_Content_Location;
        string m_Content_Range;
        string m_Content_Security_Policy;
        string m_Content_Security_Policy_Report_Only;
        string m_Content_Type;
        string m_Cookie;
        string m_Critical_CHExperimental;
        string m_Cross_Origin_Embedder_Policy;
        string m_Cross_Origin_Opener_Policy;
        string m_Cross_Origin_Resource_Policy;
        string m_Date;
        string m_Device_Memory;
        string m_Dictionary_IDExperimental;
        string m_DNTNon_standardDeprecated;
        string m_DownlinkExperimental;
        string m_DPRNon_standardDeprecated;
        string m_Early_DataExperimental;
        string m_ECTExperimental;
        string m_ETag;
        string m_Expect;
        string m_Expect_CTDeprecated;
        string m_Expires;
        string m_Forwarded;
        string m_From;
        string m_Host;
        string m_If_Match;
        string m_If_Modified_Since;
        string m_If_None_Match;
        string m_If_Range;
        string m_If_Unmodified_Since;
        string m_Keep_Alive;
        string m_Last_Modified;
        string m_Link;
        string m_Location;
        string m_Max_Forwards;
        string m_NELExperimental;
        string m_No_Vary_SearchExperimental;
        string m_Observe_Browsing_TopicsExperimentalNon_standard;
        string m_Origin;
        string m_Origin_Agent_Cluster;
        string m_Permissions_PolicyExperimental;
        string m_PragmaDeprecated;
        string m_Prefer;
        string m_Preference_Applied;
        string m_Priority;
        string m_Proxy_Authenticate;
        string m_Proxy_Authorization;
        string m_Range;
        string m_Referer;
        string m_Referrer_Policy;
        string m_Refresh;
        string m_Report_ToNon_standardDeprecated;
        string m_Reporting_Endpoints;
        string m_Repr_Digest;
        string m_Retry_After;
        string m_RTTExperimental;
        string m_Save_DataExperimental;
        string m_Sec_Browsing_TopicsExperimentalNon_standard;
        string m_Sec_CH_Prefers_Color_SchemeExperimental;
        string m_Sec_CH_Prefers_Reduced_MotionExperimental;
        string m_Sec_CH_Prefers_Reduced_TransparencyExperimental;
        string m_Sec_CH_UAExperimental;
        string m_Sec_CH_UA_ArchExperimental;
        string m_Sec_CH_UA_BitnessExperimental;
        string m_Sec_CH_UA_Form_FactorsExperimental;
        string m_Sec_CH_UA_Full_VersionDeprecated;
        string m_Sec_CH_UA_Full_Version_ListExperimental;
        string m_Sec_CH_UA_MobileExperimental;
        string m_Sec_CH_UA_ModelExperimental;
        string m_Sec_CH_UA_PlatformExperimental;
        string m_Sec_CH_UA_Platform_VersionExperimental;
        string m_Sec_CH_UA_WoW64Experimental;
        string m_Sec_Fetch_Dest;
        string m_Sec_Fetch_Mode;
        string m_Sec_Fetch_Site;
        string m_Sec_Fetch_User;
        string m_Sec_GPCExperimental;
        string m_Sec_Purpose;
        string m_Sec_Speculation_TagsExperimental;
        string m_Sec_WebSocket_Accept;
        string m_Sec_WebSocket_Extensions;
        string m_Sec_WebSocket_Key;
        string m_Sec_WebSocket_Protocol;
        string m_Sec_WebSocket_Version;
        string m_Server;
        string m_Server_Timing;
        string m_Service_Worker;
        string m_Service_Worker_Allowed;
        string m_Service_Worker_Navigation_Preload;
        string m_Set_Cookie;
        string m_Set_Login;
        string m_SourceMap;
        string m_Speculation_RulesExperimental;
        string m_Strict_Transport_Security;
        string m_Supports_Loading_ModeExperimental;
        string m_TE;
        string m_Timing_Allow_Origin;
        string m_TkNon_standardDeprecated;
        string m_Trailer;
        string m_Transfer_Encoding;
        string m_Upgrade;
        string m_Upgrade_Insecure_Requests;
        string m_Use_As_DictionaryExperimental;
        string m_User_Agent;
        string m_Vary;
        string m_Via;
        string m_Viewport_WidthNon_standardDeprecated;
        string m_Want_Content_Digest;
        string m_Want_Repr_Digest;
        string m_WarningDeprecated;
        string m_WidthNon_standardDeprecated;
        string m_WWW_Authenticate;
        string m_X_Content_Type_Options;
        string m_X_DNS_Prefetch_ControlNon_standard;
        string m_X_Forwarded_ForNon_standard;
        string m_X_Forwarded_HostNon_standard;
        string m_X_Forwarded_ProtoNon_standard;
        string m_X_Frame_Options;
        string m_X_Permitted_Cross_Domain_PoliciesNon_standard;
        string m_X_Powered_ByNon_standard;
        string m_X_Robots_TagNon_standard;
        string m_X_XSS_ProtectionNon_standardDeprecated;

    static void readHeaderLine( string line );
};

#endif // HTTPREQUEST_H_INCLUDED
